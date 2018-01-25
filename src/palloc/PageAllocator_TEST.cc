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

#include <gtest/gtest.h>
#include <prim/prim.h>

bool checkStats(const palloc::PageAllocator& _pa, u64 _freeBlocks,
                u64 _usedBlocks, u64 _freePages, u64 _usedPages) {
  u64 freeBlocks = _pa.freeBlocks();
  u64 usedBlocks = _pa.usedBlocks();
  u64 freePages = _pa.freePages();
  u64 usedPages = _pa.usedPages();
  bool pass = true;
  if (freeBlocks != _freeBlocks) {
    printf("freeBlocks: Act=%lu Exp=%lu\n", freeBlocks, _freeBlocks);
    pass = false;
  }
  if (usedBlocks != _usedBlocks) {
    printf("usedBlocks: Act=%lu Exp=%lu\n", usedBlocks, _usedBlocks);
    pass = false;
  }
  if (freePages != _freePages) {
    printf("freePages: Act=%lu Exp=%lu\n", freePages, _freePages);
    pass = false;
  }
  if (usedPages != _usedPages) {
    printf("usedPages: Act=%lu Exp=%lu\n", usedPages, _usedPages);
    pass = false;
  }
  return pass;
}


TEST(PageAllocator, createBlock) {
  u64 b0, b1, b2, b3, b4, b5, b6;
  bool verbose = false;

  palloc::PageAllocator pa(1025, 16);
  ASSERT_TRUE(checkStats(pa, 1, 0, 1025, 0));
  if (verbose) {
    pa.printBlocks();
  }

  if (verbose) {
    printf("\ncreate 0 block\n");
  }
  b0 = pa.createBlock(0);
  ASSERT_EQ(b0, palloc::INV);
  ASSERT_TRUE(checkStats(pa, 1, 0, 1025, 0));
  if (verbose) {
    pa.printBlocks();
  }

  if (verbose) {
    printf("\ncreate 16 block\n");
  }
  b1 = pa.createBlock(16);
  ASSERT_NE(b1, palloc::INV);
  ASSERT_TRUE(checkStats(pa, 1, 1, 1009, 16));
  if (verbose) {
    pa.printBlocks();
  }

  if (verbose) {
    printf("\nfree 16 block\n");
  }
  ASSERT_TRUE(pa.freeBlock(b1));
  ASSERT_TRUE(checkStats(pa, 1, 0, 1025, 0));
  if (verbose) {
    pa.printBlocks();
  }

  if (verbose) {
    printf("\nfree 16 block (again)\n");
  }
  ASSERT_FALSE(pa.freeBlock(b1));
  ASSERT_TRUE(checkStats(pa, 1, 0, 1025, 0));
  if (verbose) {
    pa.printBlocks();
  }

  if (verbose) {
    printf("\ncreate 16 block\n");
  }
  b1 = pa.createBlock(16);
  ASSERT_NE(b1, palloc::INV);
  ASSERT_TRUE(checkStats(pa, 1, 1, 1009, 16));
  if (verbose) {
    pa.printBlocks();
  }

  if (verbose) {
    printf("\ncreate 64 block\n");
  }
  b2 = pa.createBlock(64);
  ASSERT_NE(b2, palloc::INV);
  ASSERT_TRUE(checkStats(pa, 1, 2, 945, 80));
  if (verbose) {
    pa.printBlocks();
  }

  if (verbose) {
    printf("\ncreate 1 block\n");
  }
  b3 = pa.createBlock(1);
  ASSERT_NE(b3, palloc::INV);
  ASSERT_TRUE(checkStats(pa, 1, 3, 929, 96));
  if (verbose) {
    pa.printBlocks();
  }

  if (verbose) {
    printf("\nfree 64 block\n");
  }
  ASSERT_TRUE(pa.freeBlock(b2));
  ASSERT_TRUE(checkStats(pa, 2, 2, 993, 32));
  if (verbose) {
    pa.printBlocks();
  }

  if (verbose) {
    printf("\nfree 64 block (again)\n");
  }
  ASSERT_FALSE(pa.freeBlock(b2));
  ASSERT_TRUE(checkStats(pa, 2, 2, 993, 32));
  if (verbose) {
    pa.printBlocks();
  }

  if (verbose) {
    printf("\ncreate 100 block\n");
  }
  b4 = pa.createBlock(100);
  ASSERT_NE(b4, palloc::INV);
  ASSERT_TRUE(checkStats(pa, 2, 3, 893, 132));
  if (verbose) {
    pa.printBlocks();
  }

  if (verbose) {
    printf("\ncreate 829 block\n");
  }
  b5 = pa.createBlock(829);
  ASSERT_NE(b5, palloc::INV);
  ASSERT_TRUE(checkStats(pa, 1, 4, 64, 961));
  if (verbose) {
    pa.printBlocks();
  }

  if (verbose) {
    printf("\nfree 100 block\n");
  }
  ASSERT_TRUE(pa.freeBlock(b4));
  ASSERT_TRUE(checkStats(pa, 2, 3, 164, 861));
  if (verbose) {
    pa.printBlocks();
  }

  if (verbose) {
    printf("\nfree 1 block\n");
  }
  ASSERT_TRUE(pa.freeBlock(b3));
  ASSERT_TRUE(checkStats(pa, 1, 2, 180, 845));
  if (verbose) {
    pa.printBlocks();
  }
}
