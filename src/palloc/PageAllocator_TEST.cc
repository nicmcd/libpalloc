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

#include <algorithm>

TEST(PageAllocator, full) {
  u64 b0, b1, b2, b3, b4, b5;
  bool verbose = false;

  const u64 pages = 1025;  // don't change this!
  for (u64 mbs = 1; mbs <= 129; mbs++) {
    if (verbose) {
      printf("\n\n*** MBS=%lu ***\n\n\n", mbs);
    }

    palloc::PageAllocator pa(pages, mbs);
    pa.verify(verbose);

    if (verbose) {
      printf("\ncreate 0 block\n");
    }
    b0 = pa.createBlock(0);
    ASSERT_EQ(b0, palloc::INV);
    pa.verify(verbose);

    if (verbose) {
      printf("\ncreate 16 block\n");
    }
    b1 = pa.createBlock(16);
    ASSERT_NE(b1, palloc::INV);
    pa.verify(verbose);

    if (verbose) {
      printf("\nfree 16 block\n");
    }
    ASSERT_TRUE(pa.freeBlock(b1));
    pa.verify(verbose);

    if (verbose) {
      printf("\nfree 16 block (again)\n");
    }
    ASSERT_FALSE(pa.freeBlock(b1));
    pa.verify(verbose);

    if (verbose) {
      printf("\ncreate 16 block\n");
    }
    b1 = pa.createBlock(16);
    ASSERT_NE(b1, palloc::INV);
    pa.verify(verbose);

    if (verbose) {
      printf("\ncreate 64 block\n");
    }
    b2 = pa.createBlock(64);
    ASSERT_NE(b2, palloc::INV);
    pa.verify(verbose);

    if (verbose) {
      printf("\ncreate 1 block\n");
    }
    b3 = pa.createBlock(1);
    ASSERT_NE(b3, palloc::INV);
    pa.verify(verbose);

    if (verbose) {
      printf("\nfree 64 block\n");
    }
    ASSERT_TRUE(pa.freeBlock(b2));
    pa.verify(verbose);

    if (verbose) {
      printf("\nfree 64 block (again)\n");
    }
    ASSERT_FALSE(pa.freeBlock(b2));
    pa.verify(verbose);

    if (verbose) {
      printf("\ncreate 100 block\n");
    }
    b4 = pa.createBlock(100);
    ASSERT_NE(b4, palloc::INV);
    pa.verify(verbose);

    if (verbose) {
      printf("\ncreate 400 block\n");
    }
    b5 = pa.createBlock(400);
    ASSERT_NE(b5, palloc::INV);
    pa.verify(verbose);

    if (verbose) {
      printf("\nfree 100 block\n");
    }
    ASSERT_TRUE(pa.freeBlock(b4));
    pa.verify(verbose);

    if (verbose) {
      printf("\nfree 1 block\n");
    }
    ASSERT_TRUE(pa.freeBlock(b3));
    pa.verify(verbose);

    if (verbose) {
      printf("\ngrow 16 block by 3\n");
    }
    ASSERT_TRUE(pa.growBlock(b1, 19));
    pa.verify(verbose);

    if (verbose) {
      printf("\ngrow 19 block by 400\n");
    }
    ASSERT_FALSE(pa.growBlock(b1, 419));
    pa.verify(verbose);

    if (verbose) {
      printf("\ngrow 19 block by 150\n");
    }
    ASSERT_TRUE(pa.growBlock(b1, 169));
    pa.verify(verbose);

    if (verbose) {
      printf("\nshrink 196 block by 150\n");
    }
    ASSERT_TRUE(pa.shrinkBlock(b1, 46));
    pa.verify(verbose);
  }
}
