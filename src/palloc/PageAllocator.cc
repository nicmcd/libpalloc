/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * - Neither the name of prim nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific prior
 * written permission.
 *
 * See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "palloc/PageAllocator.h"

#include <bits/bits.h>

#include <cassert>
#include <cmath>

#include <algorithm>

namespace palloc {

PageAllocator::PageAllocator(u64 _pages, u64 _minBlockSize)
    : pages_(_pages), minBlockSize_(_minBlockSize) {
  // check input parameters
  if (pages_ == 0 || pages_ == INV) {
    throw new ex::Exception("pages must > 0 and < INV (%lu)", INV);
  }
  if (minBlockSize_ == 0 || minBlockSize_ > pages_) {
    throw new ex::Exception("minBlockSize must be > 0 and "
                            "minBlockSize <= pages");
  }

  // create lists
  numFreeLists_ = (u64)(std::ceil(std::log2(pages_)));
  numFreeLists_ -= (u64)(std::ceil(std::log2(minBlockSize_)) - 1);
  freeLists_.resize(numFreeLists_);
  freeListSizes_.resize(numFreeLists_);
  u64 curSize = bits::ceilPow2(minBlockSize_);
  for (u64 i = 0; i < numFreeLists_; i++) {
    freeListSizes_.at(i) = curSize;
    curSize *= 2;
  }
  freeListSizes_.at(numFreeLists_ - 1) = U64_MAX;

  // initialize the entire memory space as one large free block
  Block* freeBlock = new Block(0, pages_, false, nullptr, nullptr);
  linkFreeBlock(freeBlock);

  // initialize status counters
  freeBlocks_ = 1;
  usedBlocks_ = 0;
  freePages_ = pages_;
  usedPages_ = 0;
}

PageAllocator::~PageAllocator() {
  // delete used blocks
  for (auto& p : usedMap_) {
    delete p.second;
  }

  // delete free blocks
  for (u64 idx = 0; idx < numFreeLists_; idx++) {
    while (freeLists_.at(idx).size() > 0) {
      delete freeLists_.at(idx).front();
      freeLists_.at(idx).pop_front();
    }
  }
}

u64 PageAllocator::createBlock(u64 _pages) {
  // bail out if user is asking for nothing
  if (_pages == 0) {
    return INV;
  }

  // determine the real size of the block
  u64 pages = std::max(_pages, minBlockSize_);

  // find a free block to use
  Block* newBlock = nullptr;
  for (u64 listIndex = freeListIndex(pages); listIndex < numFreeLists_;
       listIndex++) {
    // walk the free list and use the first block large enough
    std::list<Block*>& freeList = freeLists_.at(listIndex);
    for (auto it = freeList.begin(); it != freeList.end(); ++it) {
      // check if this block is big enough, if so take it
      Block* block = *it;
      assert(block->used == false);
      if (block->size >= pages) {
        newBlock = block;
        freeList.erase(it);
        goto found;
      }
    }
  }
found:

  // detect failure to find eligible block
  if (newBlock == nullptr) {
    return INV;
  }

  // perform accounting
  freeBlocks_ -= 1;
  usedBlocks_ += 1;
  freePages_ -= newBlock->size;
  usedPages_ += newBlock->size;

  // add block to the used map
  newBlock->used = true;
  assert(usedMap_.count(newBlock->base) == 0);
  usedMap_[newBlock->base] = newBlock;

  // split the block
  splitBlock(newBlock, pages, false);  // coalescing isn't need here

  // return the block
  return newBlock->base;
}

bool PageAllocator::freeBlock(u64 _block) {
  // check if the block is a valid used block
  if (_block == INV || usedMap_.count(_block) < 1) {
    return false;
  }
  Block* block = usedMap_.at(_block);

  // remove block from used map
  usedMap_.erase(_block);
  block->used = false;

  // accounting
  freeBlocks_ += 1;
  usedBlocks_ -= 1;
  freePages_ += block->size;
  usedPages_ -= block->size;

  // coalesce free block
  coalesceBlockBackward(block);
  coalesceBlockForward(block);

  // link the free block in a free list
  linkFreeBlock(block);

  // return success
  return true;
}

bool PageAllocator::shrinkBlock(u64 _block, u64 _pages) {
  // check if the block is a valid used block
  if (_block == INV || usedMap_.count(_block) < 1) {
    return false;
  }
  Block* block = usedMap_.at(_block);

  // check easy cases
  if (_pages > block->size) {
    // can't grow
    return false;
  } else if (_pages == block->size) {
    // can stay the same
    return true;
  } else if (_pages == 0) {
    // zero is free
    return freeBlock(_block);
  }

  // split the block
  splitBlock(block, _pages, true);  // attempt to coalesce
  return true;
}

bool PageAllocator::growBlock(u64 _block, u64 _pages) {
  // check if the block is a valid used block
  if (_block == INV || usedMap_.count(_block) < 1) {
    return false;
  }
  Block* block = usedMap_.at(_block);

  // check easy cases
  if (_pages < block->size) {
    // can't shrink, but user might already have more than they asked for
    return true;
  } else if (_pages == block->size) {
    // can stay the same
    return true;
  }

  // check if the adjacent block forward is free and if the combined
  //  space would be enough
  Block* nextBlock = block->next;
  if ((nextBlock == nullptr) ||
      (nextBlock->used == true) ||
      (block->size + nextBlock->size < _pages)) {
    // consuming the next block won't work
    return false;
  }

  // coalesce the next block
  freePages_ -= nextBlock->size;
  usedPages_ += nextBlock->size;
  //  this unlinks, changes size, deletes, and block accounting
  bool coalesced = coalesceBlockForward(block);
  (void)coalesced;  // ununsed
  assert(coalesced);
  assert(block->size >= _pages);

  // split the block
  splitBlock(block, _pages, false);  // coalescing isn't need here

  // return the block
  return true;
}

u64 PageAllocator::totalBlocks() const {
  return freeBlocks_ + usedBlocks_;
}

u64 PageAllocator::freeBlocks() const {
  return freeBlocks_;
}

u64 PageAllocator::usedBlocks() const {
  return usedBlocks_;
}

u64 PageAllocator::totalPages() const {
  return freePages_ + usedPages_;
}

u64 PageAllocator::freePages() const {
  return freePages_;
}

u64 PageAllocator::usedPages() const {
  return usedPages_;
}

void PageAllocator::verify(bool _print) const {
  // find any block
  Block* block = nullptr;
  if (usedMap_.size() > 0) {
    block = usedMap_.begin()->second;
  } else {
    for (u64 idx = 0; idx < numFreeLists_; idx++) {
      if (freeLists_.at(idx).size() > 0) {
        block = *freeLists_.at(idx).begin();
        break;
      }
    }
    assert(block != nullptr);
  }

  // rewind until block is head
  while (block->prev != nullptr) {
    block = block->prev;
  }

  // scan all blocks forward
  if (_print) {
    printf("blocks in page order:\n");
  }
  std::vector<Block*> forwardBlocks;
  u64 unusedCount1 = 0;
  do {
    if (block->used == false) {
      unusedCount1++;
    }
    forwardBlocks.push_back(block);
    if (_print) {
      printf("this=0x%lX base=%lu size=%lu used=%u prev=0x%lX next=0x%lX\n",
             (u64)block, block->base, block->size, block->used,
             (u64)block->prev, (u64)block->next);
    }
    block = block->next;
  } while (block != nullptr);
  assert(forwardBlocks.size() == totalBlocks());

  // scan all blocks backward, verifying
  block = forwardBlocks.at(forwardBlocks.size() - 1);
  do {
    assert(block == forwardBlocks.at(forwardBlocks.size() - 1));
    forwardBlocks.pop_back();
    block = block->prev;
  } while (block != nullptr);

  // print free lists
  if (_print) {
    printf("free lists:\n");
  }
  u64 unusedCount2 = 0;
  for (u64 listIndex = 0; listIndex < freeLists_.size(); listIndex++) {
    u64 listSize = freeListSizes_.at(listIndex);
    if (_print) {
      printf("listIndex=%lu listSize=%lu\n", listIndex, listSize);
    }
    const std::list<Block*>& freeList = freeLists_.at(listIndex);
    for (auto it = freeList.cbegin(); it != freeList.cend(); ++it) {
      Block* block = *it;
      unusedCount2++;
      if (_print) {
        printf("this=0x%lX base=%lu size=%lu used=%u prev=0x%lX next=0x%lX\n",
               (u64)block, block->base, block->size, block->used,
               (u64)block->prev, (u64)block->next);
      }
      assert(block->used == false);
      assert(block->size <= listSize);
    }
  }
  assert(unusedCount1 == unusedCount2);
}

/*** private below here ***/

PageAllocator::Block::Block(
    u64 _base, u64 _size, bool _used, Block* _prev, Block* _next)
    : base(_base), size(_size), used(_used), prev(_prev), next(_next) {}

u64 PageAllocator::freeListIndex(u64 _pages) const {
  // printf("pages=%lu\n", _pages);
  for (u64 curIndex = 0; curIndex < numFreeLists_; curIndex++) {
    u64 curSize = freeListSizes_.at(curIndex);
    // printf("curSize=%lu\n", curSize);
    if (_pages <= curSize) {
      // printf("found\n");
      return curIndex;
    }
  }
  assert(false);
}

void PageAllocator::linkFreeBlock(Block* _block) {
  // get the block's free list index
  u64 listIndex = freeListIndex(_block->size);

  // put the block in the list in ascending size order
  std::list<Block*>& freeList = freeLists_.at(listIndex);
  for (auto it = freeList.begin(); true; ++it) {
    if (it == freeList.end() || _block->size <= (*it)->size) {
      freeList.insert(it, _block);
      break;
    }
  }
}

void PageAllocator::unlinkFreeBlock(Block* _block) {
  // get the block's free list index
  u64 listIndex = freeListIndex(_block->size);

  // remove the block from the list
  std::list<Block*>& freeList = freeLists_.at(listIndex);
  for (auto it = freeList.begin(); it != freeList.end(); ++it) {
    if (_block == *it) {
      freeList.erase(it);
      return;
    }
  }
  assert(false);
}

void PageAllocator::splitBlock(Block* _block, u64 _pages, bool _coalesce) {
  assert(_block->size >= _pages);
  u64 freeSize = _block->size - _pages;

  if (freeSize >= minBlockSize_) {
    // create the new free block
    Block* freeBlock = new Block(_block->base + _pages, freeSize, false, _block,
                                 _block->next);
    // shrink the existing used block
    _block->size = _pages;
    _block->next = freeBlock;
    if (freeBlock->next) {
      freeBlock->next->prev = freeBlock;
    }

    // accounting
    freeBlocks_ += 1;
    freePages_ += freeSize;
    usedPages_ -= freeSize;

    // coalesce (forward only)
    if (_coalesce) {
      coalesceBlockForward(freeBlock);
    }

    // link the free block in a free list
    linkFreeBlock(freeBlock);
  }
}

bool PageAllocator::coalesceBlockForward(Block* _block) {
  // get the block next to this one in the forward direction
  Block* nextBlock = _block->next;
  if (nextBlock != nullptr && nextBlock->used == false) {
    // unlink the block to be coalesced with this one
    unlinkFreeBlock(nextBlock);

    // consume the next block
    _block->size += nextBlock->size;
    _block->next = nextBlock->next;
    if (_block->next) {
      _block->next->prev = _block;
    }
    delete nextBlock;

    // accouting
    freeBlocks_ -= 1;
    return true;
  }
  return false;
}

bool PageAllocator::coalesceBlockBackward(Block* _block) {
  // get the block previous to this one in the backward direction
  Block* prevBlock = _block->prev;
  if (prevBlock != nullptr && prevBlock->used == false) {
    // unlink the block to be coalesced with this one
    unlinkFreeBlock(prevBlock);

    // consume the previous block
    _block->base = prevBlock->base;
    _block->size += prevBlock->size;
    _block->prev = prevBlock->prev;
    if (_block->prev) {
      _block->prev->next = _block;
    }
    delete prevBlock;

    // accounting
    freeBlocks_ -= 1;
    return true;
  }
  return false;
}

}  // namespace palloc
