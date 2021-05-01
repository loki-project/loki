// Copyright (c) 2014-2019, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include <cstddef>
#include <ostream>
#include <type_traits>

#include "hash_type.h"
#include "common/hex.h"
#include "crypto/cn_heavy_hash.hpp"

namespace crypto {

  extern "C" {
#include "hash-ops.h"
  }

  static_assert(HASH_SIZE == hash::size);
  static_assert(std::has_unique_object_representations_v<hash>);

  /*
    Cryptonight hash functions
  */

  inline void cn_fast_hash(const void *data, std::size_t length, hash &hash) {
    cn_fast_hash(data, length, reinterpret_cast<unsigned char*>(&hash));
  }

  inline hash cn_fast_hash(const void *data, std::size_t length) {
    hash h;
    cn_fast_hash(data, length, h);
    return h;
  }

  inline void cn_fast_hash(std::string_view data, hash& hash) {
      return cn_fast_hash(data.data(), data.size(), hash);
  }

  inline hash cn_fast_hash(std::string_view data) {
      return cn_fast_hash(data.data(), data.size());
  }

  enum struct cn_slow_hash_type
  {
#ifdef ENABLE_MONERO_SLOW_HASH
    // NOTE: Monero's slow hash for Android only, we still use the old hashing algorithm for hashing the KeyStore containing private keys
    cryptonight_v0,
    cryptonight_v0_prehashed,
    cryptonight_v1_prehashed,
#endif

    heavy_v1,
    heavy_v2,
    turtle_lite_v2,
  };

  inline void cn_slow_hash(const void *data, std::size_t length, hash &hash, cn_slow_hash_type type) {
    switch(type)
    {
      case cn_slow_hash_type::heavy_v1:
      case cn_slow_hash_type::heavy_v2:
      {
        static thread_local cn_heavy_hash_v2 v2;
        static thread_local cn_heavy_hash_v1 v1 = cn_heavy_hash_v1::make_borrowed(v2);

        if (type == cn_slow_hash_type::heavy_v1) v1.hash(data, length, hash.data);
        else                                     v2.hash(data, length, hash.data);
      }
      break;

#ifdef ENABLE_MONERO_SLOW_HASH
      case cn_slow_hash_type::cryptonight_v0:
      case cn_slow_hash_type::cryptonight_v1_prehashed:
      {
        int variant = 0, prehashed = 0;
        if (type == cn_slow_hash_type::cryptonight_v1_prehashed)
        {
          prehashed = 1;
          variant   = 1;
        }
        else if (type == cn_slow_hash_type::cryptonight_v0_prehashed)
        {
          prehashed = 1;
        }

        cn_monero_hash(data, length, hash.data, variant, prehashed);
      }
      break;
#endif

      case cn_slow_hash_type::turtle_lite_v2:
      default:
      {
         const uint32_t CN_TURTLE_SCRATCHPAD = 262144;
         const uint32_t CN_TURTLE_ITERATIONS = 131072;
         cn_turtle_hash(data,
             length,
             hash,
             1, // light
             2, // variant
             0, // pre-hashed
             CN_TURTLE_SCRATCHPAD, CN_TURTLE_ITERATIONS);
      }
      break;
    }
  }

  inline hash tree_hash(const std::vector<hash>& hashes) {
    assert(!hashes.empty());
    hash root_hash;
    tree_hash(hashes.front(), hashes.size(), root_hash);
    return root_hash;
  }

  inline std::ostream &operator <<(std::ostream &o, const crypto::hash &v) {
    return o << '<' << tools::type_to_hex(v) << '>';
  }
  inline std::ostream &operator <<(std::ostream &o, const crypto::hash8 &v) {
    return o << '<' << tools::type_to_hex(v) << '>';
  }
}
