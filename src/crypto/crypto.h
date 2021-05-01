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
#include <iostream>
#include <limits>
#include <optional>
#include <type_traits>
#include <vector>
#include <random>

#include "epee/memwipe.h"
#include "epee/mlocker.h"
#include "common/hex.h"
#include "hash.h"

#include <sodium/crypto_verify_32.h>
#include <sodium/randombytes.h>

namespace crypto {

  // Some 0s for us to compare things against.
  inline constexpr std::byte zero32[32] = {};

  struct alignas(size_t) ec_point {
    std::byte data[32];
    // Returns true if non-null, i.e. not 0.
    explicit operator bool() const { return memcmp(data, zero32, sizeof(data)); }

    // Implicit unsigned char* conversion operators for easily passing into libsodium functions.
    // (This goes through template deduction so that it only applies if we're looking for *exactly*
    // an unsigned char*, but not in places where an unsigned char* would just happen to work.  For
    // example, we don't want (a != b) to go via the implicit conversion).
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, unsigned char*>>>
    operator T() { return reinterpret_cast<unsigned char*>(data); }
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, const unsigned char*>>>
    operator T() const { return reinterpret_cast<const unsigned char*>(data); }
  };

  template <typename T1, typename T2> using is_same_point_type = std::enable_if_t<std::is_base_of_v<ec_point, T1> && std::is_same_v<T1, T2> && sizeof(T1) == sizeof(ec_point)>;

  // Equality, inequality, and less-than comparison between ec_point or subclasses. This is only
  // allowed if both arguments are of the same type.
  template <typename T1, typename T2, typename = is_same_point_type<T1, T2>>
  bool operator==(const T1& a, const T2& b) { return memcmp(a.data, b.data, sizeof(ec_point)) == 0; }
  template <typename T1, typename T2, typename = is_same_point_type<T1, T2>>
  bool operator!=(const T1& a, const T2& b) { return !(a == b); }
  template <typename T1, typename T2, typename = is_same_point_type<T1, T2>>
  bool operator<(const T1& a, const T2& b) { return memcmp(a.data, b.data, sizeof(ec_point)) < 0; }

  struct alignas(size_t) ec_scalar {
    std::byte data[32];

    // Implicit unsigned char* conversion operators for easily passing into libsodium functions.
    // See ec_point for why the templates are here.
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, unsigned char*>>>
    operator T() { return reinterpret_cast<unsigned char*>(data); }
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, const unsigned char*>>>
    operator T() const { return reinterpret_cast<const unsigned char*>(data); }
  };

  struct public_key : ec_point {
    static const public_key null;
  };
  inline constexpr public_key public_key::null{};

  struct secret_key : epee::mlocked<tools::scrubbed<ec_scalar>> {
    static const secret_key null;

    // constant-time == comparison
    bool operator==(const secret_key& x) const { return crypto_verify_32(*this, x) == 0; }
    bool operator!=(const secret_key& x) const { return !(*this == x); }

    explicit operator bool() const { return *this != null; }
  };
  inline const secret_key secret_key::null{};

  struct key_derivation : ec_point {};

  struct key_image : ec_point {
    static const key_image null;
  };
  inline constexpr key_image key_image::null{};

  struct signature {
    ec_scalar c, r;

    static const signature null;

    bool operator==(const signature& x) const { return !memcmp(this, &x, sizeof(*this)); }
    bool operator!=(const signature& x) const { return !(*this == x); }

    // Returns true if non-null, i.e. not 0.
    explicit operator bool() const { return *this != null; }
  };
  inline constexpr signature signature::null{};

  // The sizes below are all provided by sodium.h, but we don't want to depend on it here; we check
  // that they agree with the actual constants from sodium.h when compiling cryptonote_core.cpp.
  struct alignas(size_t) ed25519_public_key {
    std::byte data[32]; // 32 = crypto_sign_ed25519_PUBLICKEYBYTES

    static const ed25519_public_key null;

    bool operator==(const ed25519_public_key& x) const { return !memcmp(this, &x, sizeof(*this)); }
    bool operator!=(const ed25519_public_key& x) const { return !(*this == x); }

    /// Returns true if non-null
    explicit operator bool() const { return *this != null; }

    // Implicit conversion to unsigned char* for easier passing into libsodium functions. See
    // ec_point regarding the template usage.
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, unsigned char*>>>
    operator T() { return reinterpret_cast<unsigned char*>(data); }
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, const unsigned char*>>>
    operator T() const { return reinterpret_cast<const unsigned char*>(data); }
  };
  inline constexpr ed25519_public_key ed25519_public_key::null{};

  struct alignas(size_t) ed25519_secret_key_ {
    // 64 = crypto_sign_ed25519_SECRETKEYBYTES (but we don't depend on libsodium header here)
    std::byte data[64];
    // Implicit conversion to unsigned char* for easier passing into libsodium functions
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, unsigned char*>>>
    operator T() { return reinterpret_cast<unsigned char*>(data); }
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, const unsigned char*>>>
    operator T() const { return reinterpret_cast<const unsigned char*>(data); }
  };
  using ed25519_secret_key = epee::mlocked<tools::scrubbed<ed25519_secret_key_>>;

  struct alignas(size_t) ed25519_signature {
    std::byte data[64]; // 64 = crypto_sign_BYTES
    static const ed25519_signature null;

    // Returns true if non-null, i.e. not 0.
    explicit operator bool() const { return memcmp(this, &null, sizeof(null)); }

    bool operator==(const ed25519_signature& x) const { return !memcmp(this, &x, sizeof(*this)); }
    bool operator!=(const ed25519_signature& x) const { return !(*this == x); }

    // Implicit conversion to unsigned char* for easier passing into libsodium functions
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, unsigned char*>>>
    operator T() { return reinterpret_cast<unsigned char*>(data); }
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, const unsigned char*>>>
    operator T() const { return reinterpret_cast<const unsigned char*>(data); }
  };
  inline constexpr ed25519_signature ed25519_signature::null{};

  struct alignas(size_t) x25519_public_key {
    std::byte data[32]; // crypto_scalarmult_curve25519_BYTES
    static const x25519_public_key null;
    /// Returns true if non-null
    bool operator==(const x25519_public_key& x) const { return !memcmp(this, &x, sizeof(*this)); }
    bool operator!=(const x25519_public_key& x) const { return !(*this == x); }

    explicit operator bool() const { return memcmp(data, null.data, sizeof(null)); }

    // Implicit conversion to unsigned char* for easier passing into libsodium functions
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, unsigned char*>>>
    operator T() { return reinterpret_cast<unsigned char*>(data); }
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, const unsigned char*>>>
    operator T() const { return reinterpret_cast<const unsigned char*>(data); }
  };
  inline constexpr x25519_public_key x25519_public_key::null{};

  struct alignas(size_t) x25519_secret_key_ {
    std::byte data[32]; // crypto_scalarmult_curve25519_BYTES
    // Implicit conversion to unsigned char* for easier passing into libsodium functions
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, unsigned char*>>>
    operator T() { return reinterpret_cast<unsigned char*>(data); }
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, const unsigned char*>>>
    operator T() const { return reinterpret_cast<const unsigned char*>(data); }
  };
  using x25519_secret_key = epee::mlocked<tools::scrubbed<x25519_secret_key_>>;

  void hash_to_scalar(const void *data, size_t length, ec_scalar &res);

  static_assert(sizeof(ec_point) == 32 && sizeof(ec_scalar) == 32 &&
    sizeof(public_key) == 32 && sizeof(secret_key) == 32 &&
    sizeof(key_derivation) == 32 && sizeof(key_image) == 32 &&
    sizeof(signature) == 64, "Invalid structure size");

  /// Fill a buffer with random bytes
  template <typename T, typename = std::enable_if_t<sizeof(T) == 1>>
  void fill_random(T* buf, size_t length) {
    randombytes_buf(reinterpret_cast<unsigned char*>(buf), length);
  }

  /* Fill a trivially constructible value with random bytes.
   */
  template <typename T, typename = std::enable_if_t<!std::is_const_v<T> && !std::is_pointer_v<T> && std::has_unique_object_representations_v<T>>>
  void fill_random(T& val) {
    fill_random(reinterpret_cast<unsigned char*>(&val), sizeof(val));
  }

  /* Generate a value filled with random bytes.
   */
  template <typename T, typename = std::enable_if_t<std::has_unique_object_representations_v<T>>>
  T random_filled() {
    T res;
    fill_random(res);
    return res;
  }

  /* Trivial UniformRandomBitGenerator using libsodium's randombytes_buf.
   *
   * Note that sodium_init() must have been called (typically via a call to common/util's
   * tools::on_startup()).
   */
  struct random_device
  {
    using result_type = uint64_t;
    static constexpr result_type min() { return std::numeric_limits<result_type>::min(); }
    static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
    result_type operator()() const { return random_filled<result_type>(); }
  };
  constexpr random_device rng{};

  /* Generate a new key pair
   */
  secret_key generate_keys(public_key &pub, secret_key &sec, const secret_key& recovery_key = secret_key(), bool recover = false);

  /* Check a public key. Returns true if it is valid, false otherwise.
   */
  bool check_key(const public_key &key);

  /* Checks a private key and computes the corresponding public key.
   */
  bool secret_key_to_public_key(const secret_key &sec, public_key &pub);

  /* To generate an ephemeral key used to send money to:
   * * The sender generates a new key pair, which becomes the transaction key. The public transaction key is included in "extra" field.
   * * Both the sender and the receiver generate key derivation from the transaction key, the receivers' "view" key and the output index.
   * * The sender uses key derivation and the receivers' "spend" key to derive an ephemeral public key.
   * * The receiver can either derive the public key (to check that the transaction is addressed to him) or the private key (to spend the money).
   */
  bool generate_key_derivation(const public_key &key1, const secret_key &key2, key_derivation &derivation);
  bool derive_public_key(const key_derivation &derivation, std::size_t output_index, const public_key &base, public_key &derived_key);
  void derivation_to_scalar(const key_derivation &derivation, size_t output_index, ec_scalar &res);
  void derive_secret_key(const key_derivation &derivation, std::size_t output_index, const secret_key &base, secret_key &derived_key);
  bool derive_subaddress_public_key(const public_key &out_key, const key_derivation &derivation, std::size_t output_index, public_key &result);

  /* Generation and checking of a non-standard Monero curve 25519 signature.  This is a custom
   * scheme that is not Ed25519 because it uses a random "r" (unlike Ed25519's use of a
   * deterministic value), it requires pre-hashing the message (Ed25519 does not), and produces
   * signatures that cannot be verified using Ed25519 verification methods (because the order of
   * hashed values differs).
   *
   * Given M = H(msg)
   * r = random scalar
   * R = rG
   * c = H(M || A || R)
   *
   * and then the signature for this is:
   *
   * s = r - ac
   * Signature is: (c, s)   (but in the struct these are named "c" and "r")
   *
   * Contrast this with standard Ed25519 signature:
   *
   * Given M = msg
   * r = H(seed_hash_2nd_half || M)
   * R = rG
   * c = H(R || A || M)
   * s = r + ac
   * Signature is: (R, s)
   *
   * For verification:
   *
   * Monero: given signature (c, s), message hash M, and pubkey A:
   *
   * R = sG + cA
   * Check: H(M||A||R) == c
   *
   * Ed25519: given signature (R, s), (unhashed) message M, pubkey A:
   * Check: sB == R + H(R||A||M)A
   */
  void generate_signature(const hash &prefix_hash, const public_key &pub, const secret_key &sec, signature &sig);
  // See above.
  bool check_signature(const hash &prefix_hash, const public_key &pub, const signature &sig);

  /* Generation and checking of a tx proof; given a tx pubkey R, the recipient's view pubkey A, and the key
   * derivation D, the signature proves the knowledge of the tx secret key r such that R=r*G and D=r*A
   * When the recipient's address is a subaddress, the tx pubkey R is defined as R=r*B where B is the recipient's spend pubkey
   */
  void generate_tx_proof(const hash &prefix_hash, const public_key &R, const public_key &A, const std::optional<public_key> &B, const public_key &D, const secret_key &r, signature &sig);
  bool check_tx_proof(const hash &prefix_hash, const public_key &R, const public_key &A, const std::optional<public_key> &B, const public_key &D, const signature &sig);

  /* To send money to a key:
   * * The sender generates an ephemeral key and includes it in transaction output.
   * * To spend the money, the receiver generates a key image from it.
   * * Then he selects a bunch of outputs, including the one he spends, and uses them to generate a ring signature.
   * To check the signature, it is necessary to collect all the keys that were used to generate it. To detect double spends, it is necessary to check that each key image is used at most once.
   */
  void generate_key_image(const public_key &pub, const secret_key &sec, key_image &image);
  void generate_ring_signature(
      const hash& prefix_hash,
      const key_image& image,
      const std::vector<const public_key*>& pubs,
      const secret_key& sec,
      std::size_t sec_index,
      signature* sig);
  bool check_ring_signature(
      const hash& prefix_hash,
      const key_image& image,
      const std::vector<const public_key*>& pubs,
      const signature* sig);

  // Signature on a single key image.  The does the same thing as generate_ring_signature with 1
  // pubkey (and secret index of 0), but slightly more efficiently, and with hardware device
  // implementation.  (This is still used for key image export and for exposing key images in stake
  // transactions).
  void generate_key_image_signature(
      const key_image& image,
      const public_key& pub,
      const secret_key& sec,
      signature& sig);
  bool check_key_image_signature(
      const key_image& image,
      const public_key& pub,
      const signature& sig);

  inline std::ostream &operator <<(std::ostream &o, const crypto::public_key &v) {
    return o << '<' << tools::type_to_hex(v) << '>';
  }
  inline std::ostream &operator <<(std::ostream &o, const crypto::key_derivation &v) {
    return o << '<' << tools::type_to_hex(v) << '>';
  }
  inline std::ostream &operator <<(std::ostream &o, const crypto::key_image &v) {
    return o << '<' << tools::type_to_hex(v) << '>';
  }
  inline std::ostream &operator <<(std::ostream &o, const crypto::signature &v) {
    return o << '<' << tools::type_to_hex(v) << '>';
  }
  inline std::ostream &operator <<(std::ostream &o, const crypto::ed25519_public_key &v) {
    return o << '<' << tools::type_to_hex(v) << '>';
  }
  inline std::ostream &operator <<(std::ostream &o, const crypto::x25519_public_key &v) {
    return o << '<' << tools::type_to_hex(v) << '>';
  }

  template <typename T>
  struct already_hashed {
    std::size_t operator()(const T& v) const { return *reinterpret_cast<const std::size_t*>(&v); }
  };

  // Wrapper around crypto_core_ed25519_scalar_reduce that operates on a 32-byte value (copying it
  // into a 64-byte buffer with trailing 0's).
  void ed25519_scalar_reduce32(unsigned char* buf);

  // Stand-in object to aid with operations, most notably `scalar %= L` to reduce.
  struct ed25519_order_t {};
  inline constexpr ed25519_order_t L{};

  // Reduces something held in a 32-byte trivial type (e.g. ec_scalar, rct::key) to be mod L:
  //
  //     s %= L;
  //
  template <typename T, typename = std::enable_if_t<(std::is_trivial_v<T> || std::is_same_v<T, secret_key>) && sizeof(T) == 32>>
  T& operator%=(T& scalar, ed25519_order_t) {
    ed25519_scalar_reduce32(reinterpret_cast<unsigned char*>(&scalar));
    return scalar;
  }

  template <typename T, typename = std::enable_if_t<(std::is_trivial_v<T> || std::is_same_v<T, secret_key>) && sizeof(T) == 32>>
  T operator%(T scalar, ed25519_order_t) {
    scalar %= L;
    return scalar;
  }

}

namespace epee { template <> inline constexpr bool is_byte_spannable<crypto::secret_key> = true; }

namespace crypto {
  inline std::ostream &operator <<(std::ostream &o, const crypto::secret_key &v) {
    return o << '<' << tools::type_to_hex(v) << '>';
  }
}

namespace std {
  template<> struct hash<crypto::secret_key> : crypto::already_hashed<crypto::secret_key> {};
  template<> struct hash<crypto::public_key> : crypto::already_hashed<crypto::public_key> {};
  template<> struct hash<crypto::key_image> : crypto::already_hashed<crypto::key_image> {};
  template<> struct hash<crypto::signature> : crypto::already_hashed<crypto::signature> {};
  template<> struct hash<crypto::ed25519_public_key> : crypto::already_hashed<crypto::ed25519_public_key> {};
  template<> struct hash<crypto::x25519_public_key> : crypto::already_hashed<crypto::x25519_public_key> {};
}
