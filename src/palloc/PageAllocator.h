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
#ifndef PALLOC_PAGEALLOCATOR_H_
#define PALLOC_PAGEALLOCATOR_H_

#include <ex/Exception.h>
#include <prim/prim.h>

#include <list>
#include <unordered_map>
#include <vector>

namespace palloc {

const u64 INV = U64_MAX;

/*
 * This is an implementation of an exponential (i.e., powers of 2), sorted
 * (e.g., best memory utilization) segregated fit free list allocator.
 * Unlike common allocators, the metadata is stored in this class, not
 * in the memory itself.
 */

class PageAllocator {
 public:
  PageAllocator(u64 _pages, u64 _minBlockSize);
  ~PageAllocator();

  // allocates a block
  //  returns the base page of the block
  u64 createBlock(u64 _pages);

  // frees an allocated block
  //  returns true if success, false otherwise
  bool freeBlock(u64 _block);

  // shrinks an allocated block
  //  'pages' is the total requested size
  //  returns true if success, false otherwise
  bool shrinkBlock(u64 _block, u64 _pages);

  // grows an allocated block
  //  'pages' is the total requested size
  //  Note: this does not do any page migration
  //  returns the base page of the block if success, INV otherwise
  bool growBlock(u64 _block, u64 _pages);

  // returns the total number of blocks
  u64 totalBlocks() const;

  // returns the number of free blocks
  u64 freeBlocks() const;

  // returns the number of used blocks
  u64 usedBlocks() const;

  // returns the total number of pages
  u64 totalPages() const;

  // returns the number of free pages
  u64 freePages() const;

  // returns the number of used pages
  u64 usedPages() const;

  // verify internal data structures
  void verify(bool _print) const;

 private:
  struct Block {
    Block(u64 _base, u64 _size, bool _used, Block* _prev, Block* _next);
    u64 base;  // starting page
    u64 size;  // number of pages
    bool used;
    Block* prev;
    Block* next;
  };

  // this returns the index of the corresponding free list
  u64 freeListIndex(u64 _pages) const;

  // this links a free block into its free list
  void linkFreeBlock(Block* _block);

  // this unlinks a free block from its free list
  void unlinkFreeBlock(Block* _block);

  // this splits a block into two smaller blocks if possible
  void splitBlock(Block* _block, u64 _pages, bool _coalesce);

  // this coalesces a free block in the forward direction
  //  return true if coalescing occurred, false otherwise
  bool coalesceBlockForward(Block* _block);

  // this coalesces a free block in the backward direction
  //  return true if coalescing occurred, false otherwise
  bool coalesceBlockBackward(Block* _block);

  const u64 pages_;
  const u64 minBlockSize_;

  u64 numFreeLists_;
  std::vector<std::list<Block*> > freeLists_;
  std::vector<u64> freeListSizes_;
  std::unordered_map<u64, Block*> usedMap_;

  u64 freeBlocks_;
  u64 usedBlocks_;
  u64 freePages_;
  u64 usedPages_;
};


}  // namespace palloc

#endif  // PALLOC_PAGEALLOCATOR_H_
