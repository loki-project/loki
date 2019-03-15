// Copyright (c) 2014-2018, The Monero Project
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
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/verification_context.h"
#include "cryptonote_basic/difficulty.h"
#include "crypto/hash.h"
#include "cryptonote_config.h"
#include "cryptonote_core/service_node_deregister.h"
#include "rpc/rpc_handler.h"
#include "common/varint.h"
#include "common/perf_timer.h"

#include "common/loki.h"

namespace
{
  template<typename T>
  std::string compress_integer_array(const std::vector<T> &v)
  {
    std::string s;
    s.resize(v.size() * (sizeof(T) * 8 / 7 + 1));
    char *ptr = (char*)s.data();
    for (const T &t: v)
      tools::write_varint(ptr, t);
    s.resize(ptr - s.data());
    return s;
  }

  template<typename T>
  std::vector<T> decompress_integer_array(const std::string &s)
  {
    std::vector<T> v;
    v.reserve(s.size());
    int read = 0;
    const std::string::const_iterator end = s.end();
    for (std::string::const_iterator i = s.begin(); i != end; std::advance(i, read))
    {
      T t;
      read = tools::read_varint(std::string::const_iterator(i), s.end(), t);
      CHECK_AND_ASSERT_THROW_MES(read > 0 && read <= 256, "Error decompressing data");
      v.push_back(t);
    }
    return v;
  }
}

namespace cryptonote
{
  //-----------------------------------------------
#define CORE_RPC_STATUS_OK   "OK"
#define CORE_RPC_STATUS_BUSY   "BUSY"
#define CORE_RPC_STATUS_NOT_MINING "NOT MINING"

// When making *any* change here, bump minor
// If the change is incompatible, then bump major and set minor to 0
// This ensures CORE_RPC_VERSION always increases, that every change
// has its own version, and that clients can just test major to see
// whether they can talk to a given daemon without having to know in
// advance which version they will stop working with
// Don't go over 32767 for any of these
#define CORE_RPC_VERSION_MAJOR 2
#define CORE_RPC_VERSION_MINOR 3
#define MAKE_CORE_RPC_VERSION(major,minor) (((major)<<16)|(minor))
#define CORE_RPC_VERSION MAKE_CORE_RPC_VERSION(CORE_RPC_VERSION_MAJOR, CORE_RPC_VERSION_MINOR)


  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_HEIGHT
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t height;    // The current blockchain height according to the queried daemon
      std::string status; // Generic RPC error code. "OK" is the success value.
      bool untrusted;     // If the result is obtained using bootstrap mode, and therefore not trusted `true`, or otherwise `false`.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)
        KV_SERIALIZE(status)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_BLOCKS_FAST
  {

    struct request
    {
      std::list<crypto::hash> block_ids; //@ //*first 10 blocks id goes sequential, next goes in pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */
      uint64_t    start_height;
      bool        prune;
      bool        no_miner_tx;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(block_ids)
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(prune)
        KV_SERIALIZE_OPT(no_miner_tx, false)
      END_KV_SERIALIZE_MAP()
    };

    struct tx_output_indices
    {
      std::vector<uint64_t> indices;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(indices)
      END_KV_SERIALIZE_MAP()
    };

    struct block_output_indices
    {
      std::vector<tx_output_indices> indices;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(indices)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<block_complete_entry> blocks;
      uint64_t    start_height;
      uint64_t    current_height;
      std::string status;
      std::vector<block_output_indices> output_indices;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(blocks)
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(current_height)
        KV_SERIALIZE(status)
        KV_SERIALIZE(output_indices)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_BLOCKS_BY_HEIGHT
  {
    struct request
    {
      std::vector<uint64_t> heights;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(heights)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<block_complete_entry> blocks;
      std::string status;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(blocks)
        KV_SERIALIZE(status)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_ALT_BLOCKS_HASHES
  {
      struct request
      {
          BEGIN_KV_SERIALIZE_MAP()
          END_KV_SERIALIZE_MAP()
      };

      struct response
      {
          std::vector<std::string> blks_hashes;
          std::string status;
          bool untrusted;

          BEGIN_KV_SERIALIZE_MAP()
              KV_SERIALIZE(blks_hashes)
              KV_SERIALIZE(status)
              KV_SERIALIZE(untrusted)
          END_KV_SERIALIZE_MAP()
      };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_HASHES_FAST
  {

    struct request
    {
      std::list<crypto::hash> block_ids; //*first 10 blocks id goes sequential, next goes in pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */
      uint64_t    start_height;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(block_ids)
        KV_SERIALIZE(start_height)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<crypto::hash> m_block_ids;
      uint64_t    start_height;
      uint64_t    current_height;
      std::string status;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(m_block_ids)
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(current_height)
        KV_SERIALIZE(status)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_ADDRESS_TXS
  {
      struct request
      {
        std::string address;
        std::string view_key;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(address)
          KV_SERIALIZE(view_key)
        END_KV_SERIALIZE_MAP()
      };

      struct spent_output {
        uint64_t amount;
        std::string key_image;
        std::string tx_pub_key;
        uint64_t out_index;
        uint32_t mixin;


        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amount)
          KV_SERIALIZE(key_image)
          KV_SERIALIZE(tx_pub_key)
          KV_SERIALIZE(out_index)
          KV_SERIALIZE(mixin)
        END_KV_SERIALIZE_MAP()
      };

      struct transaction
      {
        uint64_t id;
        std::string hash;
        uint64_t timestamp;
        uint64_t total_received;
        uint64_t total_sent;
        uint64_t unlock_time;
        uint64_t height;
        std::list<spent_output> spent_outputs;
        std::string payment_id;
        bool coinbase;
        bool mempool;
        uint32_t mixin;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(id)
          KV_SERIALIZE(hash)
          KV_SERIALIZE(timestamp)
          KV_SERIALIZE(total_received)
          KV_SERIALIZE(total_sent)
          KV_SERIALIZE(unlock_time)
          KV_SERIALIZE(height)
          KV_SERIALIZE(spent_outputs)
          KV_SERIALIZE(payment_id)
          KV_SERIALIZE(coinbase)
          KV_SERIALIZE(mempool)
          KV_SERIALIZE(mixin)
        END_KV_SERIALIZE_MAP()
      };


      struct response
      {
        //std::list<std::string> txs_as_json;
        uint64_t total_received;
        uint64_t total_received_unlocked = 0; // OpenMonero only
        uint64_t scanned_height;
        std::vector<transaction> transactions;
        uint64_t blockchain_height;
        uint64_t scanned_block_height;
        std::string status;
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(total_received)
          KV_SERIALIZE(total_received_unlocked)
          KV_SERIALIZE(scanned_height)
          KV_SERIALIZE(transactions)
          KV_SERIALIZE(blockchain_height)
          KV_SERIALIZE(scanned_block_height)
          KV_SERIALIZE(status)
        END_KV_SERIALIZE_MAP()
      };
  };

  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_ADDRESS_INFO
  {
      struct request
      {
        std::string address;
        std::string view_key;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(address)
          KV_SERIALIZE(view_key)
        END_KV_SERIALIZE_MAP()
      };

      struct spent_output
      {
        uint64_t amount;
        std::string key_image;
        std::string tx_pub_key;
        uint64_t  out_index;
        uint32_t  mixin;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amount)
          KV_SERIALIZE(key_image)
          KV_SERIALIZE(tx_pub_key)
          KV_SERIALIZE(out_index)
          KV_SERIALIZE(mixin)
        END_KV_SERIALIZE_MAP()
      };



      struct response
      {
        uint64_t locked_funds;
        uint64_t total_received;
        uint64_t total_sent;
        uint64_t scanned_height;
        uint64_t scanned_block_height;
        uint64_t start_height;
        uint64_t transaction_height;
        uint64_t blockchain_height;
        std::list<spent_output> spent_outputs;
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(locked_funds)
          KV_SERIALIZE(total_received)
          KV_SERIALIZE(total_sent)
          KV_SERIALIZE(scanned_height)
          KV_SERIALIZE(scanned_block_height)
          KV_SERIALIZE(start_height)
          KV_SERIALIZE(transaction_height)
          KV_SERIALIZE(blockchain_height)
          KV_SERIALIZE(spent_outputs)
        END_KV_SERIALIZE_MAP()
      };
  };

  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_UNSPENT_OUTS
  {
      struct request
      {
        std::string amount;
        std::string address;
        std::string view_key;
        // OpenMonero specific
        uint64_t mixin;
        bool use_dust;
        std::string dust_threshold;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amount)
          KV_SERIALIZE(address)
          KV_SERIALIZE(view_key)
          KV_SERIALIZE(mixin)
          KV_SERIALIZE(use_dust)
          KV_SERIALIZE(dust_threshold)
        END_KV_SERIALIZE_MAP()
      };


      struct output {
        uint64_t amount;
        std::string public_key;
        uint64_t  index;
        uint64_t global_index;
        std::string rct;
        std::string tx_hash;
        std::string tx_pub_key;
        std::string tx_prefix_hash;
        std::vector<std::string> spend_key_images;
        uint64_t timestamp;
        uint64_t height;


        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amount)
          KV_SERIALIZE(public_key)
          KV_SERIALIZE(index)
          KV_SERIALIZE(global_index)
          KV_SERIALIZE(rct)
          KV_SERIALIZE(tx_hash)
          KV_SERIALIZE(tx_pub_key)
          KV_SERIALIZE(tx_prefix_hash)
          KV_SERIALIZE(spend_key_images)
          KV_SERIALIZE(timestamp)
          KV_SERIALIZE(height)
        END_KV_SERIALIZE_MAP()
      };

      struct response
      {
        uint64_t amount;
        std::list<output> outputs;
        uint64_t per_kb_fee;
        std::string status;
        std::string reason;
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amount)
          KV_SERIALIZE(outputs)
          KV_SERIALIZE(per_kb_fee)
          KV_SERIALIZE(status)
          KV_SERIALIZE(reason)
        END_KV_SERIALIZE_MAP()
      };
  };

  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_RANDOM_OUTS
  {
      struct request
      {
        std::vector<std::string> amounts;
        uint32_t count;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amounts)
          KV_SERIALIZE(count)
        END_KV_SERIALIZE_MAP()
      };


      struct output {
        std::string public_key;
        uint64_t global_index;
        std::string rct; // 64+64+64 characters long (<rct commit> + <encrypted mask> + <rct amount>)

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(public_key)
          KV_SERIALIZE(global_index)
          KV_SERIALIZE(rct)
        END_KV_SERIALIZE_MAP()
      };

      struct amount_out {
        uint64_t amount;
        std::vector<output> outputs;
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amount)
          KV_SERIALIZE(outputs)
        END_KV_SERIALIZE_MAP()

      };

      struct response
      {
        std::vector<amount_out> amount_outs;
        std::string Error;
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amount_outs)
          KV_SERIALIZE(Error)
        END_KV_SERIALIZE_MAP()
      };
  };
  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_SUBMIT_RAW_TX
  {
      struct request
      {
        std::string address;
        std::string view_key;
        std::string tx;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(address)
          KV_SERIALIZE(view_key)
          KV_SERIALIZE(tx)
        END_KV_SERIALIZE_MAP()
      };


      struct response
      {
        std::string status;
        std::string error;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(status)
          KV_SERIALIZE(error)
        END_KV_SERIALIZE_MAP()
      };
  };
  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_LOGIN
  {
      struct request
      {
        std::string address;
        std::string view_key;
        bool create_account;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(address)
          KV_SERIALIZE(view_key)
          KV_SERIALIZE(create_account)
        END_KV_SERIALIZE_MAP()
      };


      struct response
      {
        std::string status;
        std::string reason;
        bool new_address;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(status)
          KV_SERIALIZE(reason)
          KV_SERIALIZE(new_address)
        END_KV_SERIALIZE_MAP()
      };
  };
  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_IMPORT_WALLET_REQUEST
  {
      struct request
      {
        std::string address;
        std::string view_key;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(address)
          KV_SERIALIZE(view_key)
        END_KV_SERIALIZE_MAP()
      };


      struct response
      {
        std::string payment_id;
        uint64_t import_fee;
        bool new_request;
        bool request_fulfilled;
        std::string payment_address;
        std::string status;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(payment_id)
          KV_SERIALIZE(import_fee)
          KV_SERIALIZE(new_request)
          KV_SERIALIZE(request_fulfilled)
          KV_SERIALIZE(payment_address)
          KV_SERIALIZE(status)
        END_KV_SERIALIZE_MAP()
      };
  };
  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_TRANSACTIONS
  {
    struct request
    {
      std::vector<std::string> txs_hashes;
      bool decode_as_json;
      bool prune;
      bool split;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs_hashes)
        KV_SERIALIZE(decode_as_json)
        KV_SERIALIZE_OPT(prune, false)
        KV_SERIALIZE_OPT(split, false)
      END_KV_SERIALIZE_MAP()
    };

    struct entry
    {
      std::string tx_hash;
      std::string as_hex;
      std::string pruned_as_hex;
      std::string prunable_as_hex;
      std::string prunable_hash;
      std::string as_json;
      bool in_pool;
      bool double_spend_seen;
      uint64_t block_height;
      uint64_t block_timestamp;
      std::vector<uint64_t> output_indices;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(as_hex)
        KV_SERIALIZE(pruned_as_hex)
        KV_SERIALIZE(prunable_as_hex)
        KV_SERIALIZE(prunable_hash)
        KV_SERIALIZE(as_json)
        KV_SERIALIZE(in_pool)
        KV_SERIALIZE(double_spend_seen)
        KV_SERIALIZE(block_height)
        KV_SERIALIZE(block_timestamp)
        KV_SERIALIZE(output_indices)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      // older compatibility stuff
      std::vector<std::string> txs_as_hex;  //transactions blobs as hex (old compat)
      std::vector<std::string> txs_as_json; //transactions decoded as json (old compat)

      // in both old and new
      std::vector<std::string> missed_tx;   //not found transactions

      // new style
      std::vector<entry> txs;
      std::string status;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs_as_hex)
        KV_SERIALIZE(txs_as_json)
        KV_SERIALIZE(txs)
        KV_SERIALIZE(missed_tx)
        KV_SERIALIZE(status)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_IS_KEY_IMAGE_SPENT
  {
    enum STATUS {
      UNSPENT = 0,
      SPENT_IN_BLOCKCHAIN = 1,
      SPENT_IN_POOL = 2,
    };

    struct request
    {
      std::vector<std::string> key_images;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key_images)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::vector<int> spent_status;
      std::string status;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(spent_status)
        KV_SERIALIZE(status)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES
  {
    struct request
    {
      crypto::hash txid;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_VAL_POD_AS_BLOB(txid)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::vector<uint64_t> o_indexes;
      std::string status;
      bool untrusted;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(o_indexes)
        KV_SERIALIZE(status)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------

  LOKI_RPC_DOC_INTROSPECT
  struct get_outputs_out
  {
    uint64_t amount;
    uint64_t index;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(index)
    END_KV_SERIALIZE_MAP()
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_OUTPUTS_BIN
  {
    struct request
    {
      std::vector<get_outputs_out> outputs;
      bool get_txid;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outputs)
        KV_SERIALIZE_OPT(get_txid, true)
      END_KV_SERIALIZE_MAP()
    };

    struct outkey
    {
      crypto::public_key key;
      rct::key mask;
      bool unlocked;
      uint64_t height;
      crypto::hash txid;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_VAL_POD_AS_BLOB(key)
        KV_SERIALIZE_VAL_POD_AS_BLOB(mask)
        KV_SERIALIZE(unlocked)
        KV_SERIALIZE(height)
        KV_SERIALIZE_VAL_POD_AS_BLOB(txid)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<outkey> outs;
      std::string status;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outs)
        KV_SERIALIZE(status)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_OUTPUTS
  {
    struct request
    {
      std::vector<get_outputs_out> outputs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outputs)
      END_KV_SERIALIZE_MAP()
    };

    struct outkey
    {
      std::string key;
      std::string mask;
      bool unlocked;
      uint64_t height;
      std::string txid;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key)
        KV_SERIALIZE(mask)
        KV_SERIALIZE(unlocked)
        KV_SERIALIZE(height)
        KV_SERIALIZE(txid)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<outkey> outs;
      std::string status;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outs)
        KV_SERIALIZE(status)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_SEND_RAW_TX
  {
    struct request
    {
      std::string tx_as_hex;
      bool do_not_relay;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_as_hex)
        KV_SERIALIZE_OPT(do_not_relay, false)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;
      std::string reason;
      bool not_relayed;
      bool untrusted;
      tx_verification_context tvc;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(reason)
        KV_SERIALIZE(not_relayed)
        KV_SERIALIZE(untrusted)
        KV_SERIALIZE(tvc)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_START_MINING
  {
    struct request
    {
      std::string miner_address;
      uint64_t    threads_count;
      bool        do_background_mining;
      bool        ignore_battery;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(miner_address)
        KV_SERIALIZE(threads_count)
        KV_SERIALIZE(do_background_mining)
        KV_SERIALIZE(ignore_battery)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_INFO
  {
    struct request
    {

      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      uint64_t height;
      uint64_t target_height;
      uint64_t difficulty;
      uint64_t target;
      uint64_t tx_count;
      uint64_t tx_pool_size;
      uint64_t alt_blocks_count;
      uint64_t outgoing_connections_count;
      uint64_t incoming_connections_count;
      uint64_t rpc_connections_count;
      uint64_t white_peerlist_size;
      uint64_t grey_peerlist_size;
      bool mainnet;
      bool testnet;
      bool stagenet;
      std::string nettype;
      std::string top_block_hash;
      uint64_t cumulative_difficulty;
      uint64_t block_size_limit;
      uint64_t block_weight_limit;
      uint64_t block_size_median;
      uint64_t block_weight_median;
      uint64_t start_time;
      uint64_t free_space;
      bool offline;
      bool untrusted;
      std::string bootstrap_daemon_address;
      uint64_t height_without_bootstrap;
      bool was_bootstrap_ever_used;
      uint64_t database_size;
      bool update_available;
      std::string version;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(height)
        KV_SERIALIZE(target_height)
        KV_SERIALIZE(difficulty)
        KV_SERIALIZE(target)
        KV_SERIALIZE(tx_count)
        KV_SERIALIZE(tx_pool_size)
        KV_SERIALIZE(alt_blocks_count)
        KV_SERIALIZE(outgoing_connections_count)
        KV_SERIALIZE(incoming_connections_count)
        KV_SERIALIZE(rpc_connections_count)
        KV_SERIALIZE(white_peerlist_size)
        KV_SERIALIZE(grey_peerlist_size)
        KV_SERIALIZE(mainnet)
        KV_SERIALIZE(testnet)
        KV_SERIALIZE(stagenet)
        KV_SERIALIZE(nettype)
        KV_SERIALIZE(top_block_hash)
        KV_SERIALIZE(cumulative_difficulty)
        KV_SERIALIZE(block_size_limit)
        KV_SERIALIZE_OPT(block_weight_limit, (uint64_t)0)
        KV_SERIALIZE(block_size_median)
        KV_SERIALIZE_OPT(block_weight_median, (uint64_t)0)
        KV_SERIALIZE(start_time)
        KV_SERIALIZE(free_space)
        KV_SERIALIZE(offline)
        KV_SERIALIZE(untrusted)
        KV_SERIALIZE(bootstrap_daemon_address)
        KV_SERIALIZE(height_without_bootstrap)
        KV_SERIALIZE(was_bootstrap_ever_used)
        KV_SERIALIZE(database_size)
        KV_SERIALIZE(update_available)
        KV_SERIALIZE(version)
      END_KV_SERIALIZE_MAP()
    };
  };


  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_ALL_SERVICE_NODES_KEYS
  {
    struct request
    {
      bool fully_funded_nodes_only; // Return keys for service nodes if they are funded and working on the network
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_OPT(fully_funded_nodes_only, (bool)true)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<std::string> keys; // NOTE: Returns as base32z of the hex key, for Lokinet internal usage
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(keys)
      END_KV_SERIALIZE_MAP()
    };
  };


  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_STOP_MINING
  {
    struct request
    {

      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_MINING_STATUS
  {
    struct request
    {

      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;
      bool active;
      uint64_t speed;
      uint32_t threads_count;
      std::string address;
      bool is_background_mining_enabled;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(active)
        KV_SERIALIZE(speed)
        KV_SERIALIZE(threads_count)
        KV_SERIALIZE(address)
        KV_SERIALIZE(is_background_mining_enabled)
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_SAVE_BC
  {
    struct request
    {

      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  //
  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GETBLOCKCOUNT
  {
    typedef std::list<std::string> request;

    struct response
    {
      uint64_t count;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(count)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };

  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GETBLOCKHASH
  {
    typedef std::vector<uint64_t> request;

    typedef std::string response;
  };


  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GETBLOCKTEMPLATE
  {
    struct request
    {
      uint64_t reserve_size;       //max 255 bytes
      std::string wallet_address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(reserve_size)
        KV_SERIALIZE(wallet_address)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t difficulty;
      uint64_t height;
      uint64_t reserved_offset;
      uint64_t expected_reward;
      std::string prev_hash;
      blobdata blocktemplate_blob;
      blobdata blockhashing_blob;
      std::string status;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(difficulty)
        KV_SERIALIZE(height)
        KV_SERIALIZE(reserved_offset)
        KV_SERIALIZE(expected_reward)
        KV_SERIALIZE(prev_hash)
        KV_SERIALIZE(blocktemplate_blob)
        KV_SERIALIZE(blockhashing_blob)
        KV_SERIALIZE(status)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_SUBMITBLOCK
  {
    typedef std::vector<std::string> request;

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GENERATEBLOCKS
  {
    struct request
    {
      uint64_t amount_of_blocks;
      std::string wallet_address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount_of_blocks)
        KV_SERIALIZE(wallet_address)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t height;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct block_header_response
  {
      uint8_t major_version;
      uint8_t minor_version;
      uint64_t timestamp;
      std::string prev_hash;
      uint32_t nonce;
      bool orphan_status;
      uint64_t height;
      uint64_t depth;
      std::string hash;
      difficulty_type difficulty;
      difficulty_type cumulative_difficulty;
      uint64_t reward;
      uint64_t miner_reward;
      uint64_t block_size;
      uint64_t block_weight;
      uint64_t num_txes;
      std::string pow_hash;
      uint64_t long_term_weight;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(major_version)
        KV_SERIALIZE(minor_version)
        KV_SERIALIZE(timestamp)
        KV_SERIALIZE(prev_hash)
        KV_SERIALIZE(nonce)
        KV_SERIALIZE(orphan_status)
        KV_SERIALIZE(height)
        KV_SERIALIZE(depth)
        KV_SERIALIZE(hash)
        KV_SERIALIZE(difficulty)
        KV_SERIALIZE(cumulative_difficulty)
        KV_SERIALIZE(reward)
        KV_SERIALIZE(miner_reward)
        KV_SERIALIZE(block_size)
        KV_SERIALIZE_OPT(block_weight, (uint64_t)0)
        KV_SERIALIZE(num_txes)
        KV_SERIALIZE(pow_hash)
        KV_SERIALIZE_OPT(long_term_weight, (uint64_t)0)
      END_KV_SERIALIZE_MAP()
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_LAST_BLOCK_HEADER
  {
    struct request
    {
      bool fill_pow_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_OPT(fill_pow_hash, false);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      block_header_response block_header;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_header)
        KV_SERIALIZE(status)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };

  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH
  {
    struct request
    {
      std::string hash;
      bool fill_pow_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(hash)
        KV_SERIALIZE_OPT(fill_pow_hash, false);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      block_header_response block_header;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_header)
        KV_SERIALIZE(status)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };

  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT
  {
    struct request
    {
      uint64_t height;
      bool fill_pow_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)
        KV_SERIALIZE_OPT(fill_pow_hash, false);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      block_header_response block_header;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_header)
        KV_SERIALIZE(status)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };

  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_BLOCK
  {
    struct request
    {
      std::string hash;
      uint64_t height;
      bool fill_pow_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(hash)
        KV_SERIALIZE(height)
        KV_SERIALIZE_OPT(fill_pow_hash, false);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      block_header_response block_header;
      std::string miner_tx_hash;
      std::vector<std::string> tx_hashes;
      std::string blob;
      std::string json;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_header)
        KV_SERIALIZE(miner_tx_hash)
        KV_SERIALIZE(tx_hashes)
        KV_SERIALIZE(status)
        KV_SERIALIZE(blob)
        KV_SERIALIZE(json)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };

  };

  LOKI_RPC_DOC_INTROSPECT
  struct peer {
    uint64_t id;
    std::string host;
    uint32_t ip;
    uint16_t port;
    uint64_t last_seen;
    uint32_t pruning_seed;

    peer() = default;

    peer(uint64_t id, const std::string &host, uint64_t last_seen, uint32_t pruning_seed)
      : id(id), host(host), ip(0), port(0), last_seen(last_seen), pruning_seed(pruning_seed)
    {}
    peer(uint64_t id, uint32_t ip, uint16_t port, uint64_t last_seen, uint32_t pruning_seed)
      : id(id), host(std::to_string(ip)), ip(ip), port(port), last_seen(last_seen), pruning_seed(pruning_seed)
    {}

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(id)
      KV_SERIALIZE(host)
      KV_SERIALIZE(ip)
      KV_SERIALIZE(port)
      KV_SERIALIZE(last_seen)
      KV_SERIALIZE_OPT(pruning_seed, (uint32_t)0)
    END_KV_SERIALIZE_MAP()
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_PEER_LIST
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<peer> white_list;
      std::vector<peer> gray_list;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(white_list)
        KV_SERIALIZE(gray_list)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_SET_LOG_HASH_RATE
  {
    struct request
    {
      bool visible;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(visible)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_SET_LOG_LEVEL
  {
    struct request
    {
      int8_t level;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(level)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_SET_LOG_CATEGORIES
  {
    struct request
    {
      std::string categories;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(categories)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::string categories;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(categories)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct tx_info
  {
    std::string id_hash;
    std::string tx_json; // TODO - expose this data directly
    uint64_t blob_size;
    uint64_t weight;
    uint64_t fee;
    std::string max_used_block_id_hash;
    uint64_t max_used_block_height;
    bool kept_by_block;
    uint64_t last_failed_height;
    std::string last_failed_id_hash;
    uint64_t receive_time;
    bool relayed;
    uint64_t last_relayed_time;
    bool do_not_relay;
    bool double_spend_seen;
    std::string tx_blob;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(id_hash)
      KV_SERIALIZE(tx_json)
      KV_SERIALIZE(blob_size)
      KV_SERIALIZE_OPT(weight, (uint64_t)0)
      KV_SERIALIZE(fee)
      KV_SERIALIZE(max_used_block_id_hash)
      KV_SERIALIZE(max_used_block_height)
      KV_SERIALIZE(kept_by_block)
      KV_SERIALIZE(last_failed_height)
      KV_SERIALIZE(last_failed_id_hash)
      KV_SERIALIZE(receive_time)
      KV_SERIALIZE(relayed)
      KV_SERIALIZE(last_relayed_time)
      KV_SERIALIZE(do_not_relay)
      KV_SERIALIZE(double_spend_seen)
      KV_SERIALIZE(tx_blob)
    END_KV_SERIALIZE_MAP()
  };

  LOKI_RPC_DOC_INTROSPECT
  struct spent_key_image_info
  {
    std::string id_hash;
    std::vector<std::string> txs_hashes;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(id_hash)
      KV_SERIALIZE(txs_hashes)
    END_KV_SERIALIZE_MAP()
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_TRANSACTION_POOL
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<tx_info> transactions;
      std::vector<spent_key_image_info> spent_key_images;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(transactions)
        KV_SERIALIZE(spent_key_images)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_TRANSACTION_POOL_HASHES_BIN
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<crypto::hash> tx_hashes;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(tx_hashes)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_TRANSACTION_POOL_HASHES
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<std::string> tx_hashes;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(tx_hashes)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct tx_backlog_entry
  {
    uint64_t weight;
    uint64_t fee;
    uint64_t time_in_pool;
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<tx_backlog_entry> backlog;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(backlog)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct txpool_histo
  {
    uint32_t txs;
    uint64_t bytes;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(txs)
      KV_SERIALIZE(bytes)
    END_KV_SERIALIZE_MAP()
  };

  LOKI_RPC_DOC_INTROSPECT
  struct txpool_stats
  {
    uint64_t bytes_total;
    uint32_t bytes_min;
    uint32_t bytes_max;
    uint32_t bytes_med;
    uint64_t fee_total;
    uint64_t oldest;
    uint32_t txs_total;
    uint32_t num_failing;
    uint32_t num_10m;
    uint32_t num_not_relayed;
    uint64_t histo_98pc;
    std::vector<txpool_histo> histo;
    uint32_t num_double_spends;

    txpool_stats(): bytes_total(0), bytes_min(0), bytes_max(0), bytes_med(0), fee_total(0), oldest(0), txs_total(0), num_failing(0), num_10m(0), num_not_relayed(0), histo_98pc(0), num_double_spends(0) {}

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(bytes_total)
      KV_SERIALIZE(bytes_min)
      KV_SERIALIZE(bytes_max)
      KV_SERIALIZE(bytes_med)
      KV_SERIALIZE(fee_total)
      KV_SERIALIZE(oldest)
      KV_SERIALIZE(txs_total)
      KV_SERIALIZE(num_failing)
      KV_SERIALIZE(num_10m)
      KV_SERIALIZE(num_not_relayed)
      KV_SERIALIZE(histo_98pc)
      KV_SERIALIZE_CONTAINER_POD_AS_BLOB(histo)
      KV_SERIALIZE(num_double_spends)
    END_KV_SERIALIZE_MAP()
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_TRANSACTION_POOL_STATS
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      txpool_stats pool_stats;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(pool_stats)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_CONNECTIONS
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<connection_info> connections;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(connections)
      END_KV_SERIALIZE_MAP()
    };

  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_BLOCK_HEADERS_RANGE
  {
    struct request
    {
      uint64_t start_height;
      uint64_t end_height;
      bool fill_pow_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(end_height)
        KV_SERIALIZE_OPT(fill_pow_hash, false);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<block_header_response> headers;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(headers)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_STOP_DAEMON
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_FAST_EXIT
  {
	struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
	  std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_LIMIT
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      uint64_t limit_up;
      uint64_t limit_down;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(limit_up)
        KV_SERIALIZE(limit_down)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_SET_LIMIT
  {
    struct request
    {
      int64_t limit_down;  // all limits (for get and set) are kB/s
      int64_t limit_up;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(limit_down)
        KV_SERIALIZE(limit_up)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      int64_t limit_up;
      int64_t limit_down;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(limit_up)
        KV_SERIALIZE(limit_down)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_OUT_PEERS
  {
	struct request
    {
	  uint64_t out_peers;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(out_peers)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_IN_PEERS
  {
    struct request
    {
      uint64_t in_peers;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(in_peers)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_START_SAVE_GRAPH
  {
	struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
	  std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_STOP_SAVE_GRAPH
  {
	struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
	  std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_HARD_FORK_INFO
  {
    struct request
    {
      uint8_t version;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(version)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint8_t version;
      bool enabled;
      uint32_t window;
      uint32_t votes;
      uint32_t threshold;
      uint8_t voting;
      uint32_t state;
      uint64_t earliest_height;
      std::string status;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(version)
        KV_SERIALIZE(enabled)
        KV_SERIALIZE(window)
        KV_SERIALIZE(votes)
        KV_SERIALIZE(threshold)
        KV_SERIALIZE(voting)
        KV_SERIALIZE(state)
        KV_SERIALIZE(earliest_height)
        KV_SERIALIZE(status)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GETBANS
  {
    struct ban
    {
      std::string host;
      uint32_t ip;
      uint32_t seconds;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(host)
        KV_SERIALIZE(ip)
        KV_SERIALIZE(seconds)
      END_KV_SERIALIZE_MAP()
    };

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<ban> bans;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(bans)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_SETBANS
  {
    struct ban
    {
      std::string host;
      uint32_t ip;
      bool ban;
      uint32_t seconds;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(host)
        KV_SERIALIZE(ip)
        KV_SERIALIZE(ban)
        KV_SERIALIZE(seconds)
      END_KV_SERIALIZE_MAP()
    };

    struct request
    {
      std::vector<ban> bans;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(bans)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_FLUSH_TRANSACTION_POOL
  {
    struct request
    {
      std::vector<std::string> txids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txids)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_OUTPUT_HISTOGRAM
  {
    struct request
    {
      std::vector<uint64_t> amounts;
      uint64_t min_count;
      uint64_t max_count;
      bool unlocked;
      uint64_t recent_cutoff;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amounts);
        KV_SERIALIZE(min_count);
        KV_SERIALIZE(max_count);
        KV_SERIALIZE(unlocked);
        KV_SERIALIZE(recent_cutoff);
      END_KV_SERIALIZE_MAP()
    };

    struct entry
    {
      uint64_t amount;
      uint64_t total_instances;
      uint64_t unlocked_instances;
      uint64_t recent_instances;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount);
        KV_SERIALIZE(total_instances);
        KV_SERIALIZE(unlocked_instances);
        KV_SERIALIZE(recent_instances);
      END_KV_SERIALIZE_MAP()

      entry(uint64_t amount, uint64_t total_instances, uint64_t unlocked_instances, uint64_t recent_instances):
          amount(amount), total_instances(total_instances), unlocked_instances(unlocked_instances), recent_instances(recent_instances) {}
      entry() {}
    };

    struct response
    {
      std::string status;
      std::vector<entry> histogram;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(histogram)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_VERSION
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      uint32_t version;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(version)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_COINBASE_TX_SUM
  {
    struct request
    {
      uint64_t height;
      uint64_t count;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height);
        KV_SERIALIZE(count);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      uint64_t emission_amount;
      uint64_t fee_amount;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(emission_amount)
        KV_SERIALIZE(fee_amount)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_BASE_FEE_ESTIMATE
  {
    struct request
    {
      uint64_t grace_blocks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(grace_blocks)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      uint64_t fee;
      uint64_t quantization_mask;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(fee)
        KV_SERIALIZE_OPT(quantization_mask, (uint64_t)1)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_ALTERNATE_CHAINS
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct chain_info
    {
      std::string block_hash;
      uint64_t height;
      uint64_t length;
      uint64_t difficulty;
      std::vector<std::string> block_hashes;
      std::string main_chain_parent_block;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_hash)
        KV_SERIALIZE(height)
        KV_SERIALIZE(length)
        KV_SERIALIZE(difficulty)
        KV_SERIALIZE(block_hashes)
        KV_SERIALIZE(main_chain_parent_block)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<chain_info> chains;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(chains)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_UPDATE
  {
    struct request
    {
      std::string command;
      std::string path;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(command);
        KV_SERIALIZE(path);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      bool update;
      std::string version;
      std::string user_uri;
      std::string auto_uri;
      std::string hash;
      std::string path;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(update)
        KV_SERIALIZE(version)
        KV_SERIALIZE(user_uri)
        KV_SERIALIZE(auto_uri)
        KV_SERIALIZE(hash)
        KV_SERIALIZE(path)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_RELAY_TX
  {
    struct request
    {
      std::vector<std::string> txids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txids)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_SYNC_INFO
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct peer
    {
      connection_info info;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(info)
      END_KV_SERIALIZE_MAP()
    };

    struct span
    {
      uint64_t start_block_height;
      uint64_t nblocks;
      std::string connection_id;
      uint32_t rate;
      uint32_t speed;
      uint64_t size;
      std::string remote_address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(start_block_height)
        KV_SERIALIZE(nblocks)
        KV_SERIALIZE(connection_id)
        KV_SERIALIZE(rate)
        KV_SERIALIZE(speed)
        KV_SERIALIZE(size)
        KV_SERIALIZE(remote_address)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      uint64_t height;
      uint64_t target_height;
      uint32_t next_needed_pruning_seed;
      std::list<peer> peers;
      std::list<span> spans;
      std::string overview;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(height)
        KV_SERIALIZE(target_height)
        KV_SERIALIZE(next_needed_pruning_seed)
        KV_SERIALIZE(peers)
        KV_SERIALIZE(spans)
        KV_SERIALIZE(overview)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_OUTPUT_DISTRIBUTION
  {
    struct request
    {
      std::vector<uint64_t> amounts;
      uint64_t from_height;
      uint64_t to_height;
      bool cumulative;
      bool binary;
      bool compress;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amounts)
        KV_SERIALIZE_OPT(from_height, (uint64_t)0)
        KV_SERIALIZE_OPT(to_height, (uint64_t)0)
        KV_SERIALIZE_OPT(cumulative, false)
        KV_SERIALIZE_OPT(binary, true)
        KV_SERIALIZE_OPT(compress, false)
      END_KV_SERIALIZE_MAP()
    };

    struct distribution
    {
      rpc::output_distribution_data data;
      uint64_t amount;
      std::string compressed_data;
      bool binary;
      bool compress;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount)
        KV_SERIALIZE_N(data.start_height, "start_height")
        KV_SERIALIZE(binary)
        KV_SERIALIZE(compress)
        if (this_ref.binary)
        {
          if (is_store)
          {
            if (this_ref.compress)
            {
              const_cast<std::string&>(this_ref.compressed_data) = compress_integer_array(this_ref.data.distribution);
              KV_SERIALIZE(compressed_data)
            }
            else
              KV_SERIALIZE_CONTAINER_POD_AS_BLOB_N(data.distribution, "distribution")
          }
          else
          {
            if (this_ref.compress)
            {
              KV_SERIALIZE(compressed_data)
              const_cast<std::vector<uint64_t>&>(this_ref.data.distribution) = decompress_integer_array<uint64_t>(this_ref.compressed_data);
            }
            else
              KV_SERIALIZE_CONTAINER_POD_AS_BLOB_N(data.distribution, "distribution")
          }
        }
        else
          KV_SERIALIZE_N(data.distribution, "distribution")
        KV_SERIALIZE_N(data.base, "base")
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<distribution> distributions;
      bool untrusted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(distributions)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_POP_BLOCKS
  {
    struct request
    {
      uint64_t nblocks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(nblocks);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      uint64_t height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(height)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_PRUNE_BLOCKCHAIN
  {
    struct request
    {
      bool check;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_OPT(check, false)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint32_t pruning_seed;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(pruning_seed)
      END_KV_SERIALIZE_MAP()
    };
  };

  //
  // Loki
  //
  LOKI_RPC_DOC_INTROSPECT
  // Example
  // curl -X POST http://127.0.0.1:38157/json_rpc -d '{"jsonrpc":"2.0","id":"0","method":"get_quorum_state", "params": {"height": 200}}' -H 'Content-Type: application/json'
  // {
  //   "id": "0",
  //   "jsonrpc": "2.0",
  //   "result": {
  //     "nodes_to_test":
  //        ["578e5ee53150a3276dd3c411cb6313324a63b530cf3651f5c15e3d0ca58ceddd",
  //         …
  //         "c917034e9fcd0e9b0d423638664bbfc36eb8a2eeb68a1ff8bed8be5f699bc3c0"],
  //     "quorum_nodes":
  //        ["fc86a737756b6ed9f81233d22da3baee32537f3087901c3e94384be85ca1a9ee",
  //         …
  //         "ee597c5c7bbf1452e689a785f1133fc1355889b4111955d54cb5ed826cd35a32"],
  //     "status": "OK",
  //     "untrusted": false
  //   }
  // }
  struct COMMAND_RPC_GET_QUORUM_STATE
  {
    struct request
    {
      uint64_t height; // The height to query the quorum state for.
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;                     // Generic RPC error code. "OK" is the success value.
      std::vector<std::string> quorum_nodes;  // Array of public keys identifying service nodes which are being tested for the queried height.
      std::vector<std::string> nodes_to_test; // Array of public keys identifying service nodes which are responsible for voting on the queried height.
      bool untrusted;                         // If the result is obtained using bootstrap mode, and therefore not trusted `true`, or otherwise `false`.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(quorum_nodes)
        KV_SERIALIZE(nodes_to_test)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_QUORUM_STATE_BATCHED
  {
    struct request
    {
      uint64_t height_begin; // The starting height (inclusive) to query the quorum state for.
      uint64_t height_end;   // The ending height (inclusive) to query the quorum state for.
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height_begin)
        KV_SERIALIZE(height_end)
      END_KV_SERIALIZE_MAP()
    };

    struct response_entry
    {
      uint64_t height;                        // The height of this quorum state that was queried.
      std::vector<std::string> quorum_nodes;  // Array of public keys identifying service nodes which are being tested for the queried height.
      std::vector<std::string> nodes_to_test; // Array of public keys identifying service nodes which are responsible for voting on the queried height.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)
        KV_SERIALIZE(quorum_nodes)
        KV_SERIALIZE(nodes_to_test)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;                         // Generic RPC error code. "OK" is the success value.
      std::vector<response_entry> quorum_entries; // Array of quorums that was requested.
      bool untrusted;                             // If the result is obtained using bootstrap mode, and therefore not trusted `true`, or otherwise `false`.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(quorum_entries)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_SERVICE_NODE_REGISTRATION_CMD_RAW
  {
    struct request
    {
      std::vector<std::string> args; // (Developer) The arguments used in raw registration, i.e. portions
      bool make_friendly;            // Provide information about how to use the command in the result.
      uint64_t staking_requirement;  // The staking requirement to become a Service Node the registration command will be generated upon

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(args)
        KV_SERIALIZE(make_friendly)
        KV_SERIALIZE(staking_requirement)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;           // Generic RPC error code. "OK" is the success value.
      std::string registration_cmd; // The command to execute in the wallet CLI to register the queried daemon as a Service Node.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(registration_cmd)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_SERVICE_NODE_REGISTRATION_CMD
  {
    struct contribution_t
    {
      std::string address; // The wallet address for the contributor
      uint64_t amount;     // The amount that the contributor will reserve in Loki atomic units towards the staking requirement

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(amount)
      END_KV_SERIALIZE_MAP()
    };

    struct request
    {
      std::string operator_cut;                  // The percentage of cut per reward the operator receives expressed as a string, i.e. "1.1%"
      std::vector<contribution_t> contributions; // Array of contributors for this Service Node
      uint64_t staking_requirement;              // The staking requirement to become a Service Node the registration command will be generated upon

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(operator_cut)
        KV_SERIALIZE(contributions)
        KV_SERIALIZE(staking_requirement)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;           // Generic RPC error code. "OK" is the success value.
      std::string registration_cmd; // The command to execute in the wallet CLI to register the queried daemon as a Service Node.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(registration_cmd)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  // Example
  // curl -X POST http://127.0.0.1:38157/json_rpc -d '{"jsonrpc":"2.0","id":"0","method":"get_service_node_key"}' -H 'Content-Type: application/json'
  // {
  //   "id": "0",
  //   "jsonrpc": "2.0",
  //   "result": {
  //     "service_node_pubkey": "8d56c1fa0304884e612ee2efe763b2c50991a66329418fd084a3f23c75399f34",
  //     "status": "OK"
  //   }
  // }
  struct COMMAND_RPC_GET_SERVICE_NODE_KEY
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string service_node_pubkey; // The queried daemon's service node key.
      std::string status;              // Generic RPC error code. "OK" is the success value.
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(service_node_pubkey)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  // Example
  // curl -X POST http://127.0.0.1:38157/json_rpc -d '{"jsonrpc":"2.0","id":"0","method":"get_service_nodes", “params”: {“service_node_pubkeys”: [], "include_json": false}}' -H 'Content-Type: application/json'
  // {
  //   "id": "0",
  //   "jsonrpc": "2.0",
  //   "result": {
  //     "service_node_states": [{
  //       "contributors": [{
  //         "address":    "T6T6kZfTEf5JGw2SgLrGuxRNFxRTf51fvbbcCYW949RsKjX75JMA1B1d8CT4VbwfGR8uf3f3AJSTaBHGpN3QRG2N2LyiksWVg",
  //         "amount": 100000000000,
  //         "reserved": 100000000000
  //         "locked_contributions": [{
  //             "key_image": "8d1bd8181bf7d857bdb281e0153d84cd55a3fcaa57c3e570f4a49f935850b5e3",
  //             "key_image_pub_key": "0731363c58dd4492f031fa20c82fe6ddcb9cc070d73938afe8a5f7f77897f8b4",
  //             "amount": 100000000000
  //         }]
  //       }],
  //       "last_reward_block_height": 2968,
  //       "last_reward_transaction_index": 4294967295,
  //       "last_uptime_proof": 0,
  //       "operator_address": "T6T6kZfTEf5JGw2SgLrGuxRNFxRTf51fvbbcCYW949RsKjX75JMA1B1d8CT4VbwfGR8uf3f3AJSTaBHGpN3QRG2N2LyiksWVg",
  //       "portions_for_operator": 18446744073709551612,
  //       "registration_height": 1860,
  //       "requested_unlock_height": 0,
  //       "service_node_pubkey": "3afa36a4855a429f5eac1b2f8e7e77657a2e862999ab4d59e473826f9b15f2da",
  //       "staking_requirement": 100000000000,
  //       "total_contributed": 100000000000,
  //       "total_reserved": 100000000000
  //     }],
  //     "status": "OK",
  //     "as_json": ""
  //   }
  // }
  struct COMMAND_RPC_GET_SERVICE_NODES
  {
    struct request
    {
      std::vector<std::string> service_node_pubkeys; // Array of public keys of active Service Nodes to get information about. Pass the empty array to query all Service Nodes.
      bool include_json;                             // When set, the response's as_json member is filled out.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(service_node_pubkeys);
        KV_SERIALIZE(include_json);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      struct contribution
      {
        std::string key_image;         // The contribution's key image that is locked on the network.
        std::string key_image_pub_key; // The contribution's key image, public key component
        uint64_t    amount;            // The amount that is locked in this contribution.

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(key_image)
          KV_SERIALIZE(key_image_pub_key)
          KV_SERIALIZE(amount)
        END_KV_SERIALIZE_MAP()
      };

      struct contributor
      {
        uint64_t amount;                                // The total amount of locked Loki in atomic units for this contributor.
        uint64_t reserved;                              // The amount of Loki in atomic units reserved by this contributor for this Service Node.
        std::string address;                            // The wallet address for this contributor rewards are sent to and contributions came from.
        std::vector<contribution> locked_contributions; // Array of contributions from this contributor.

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amount)
          KV_SERIALIZE(reserved)
          KV_SERIALIZE(address)
          KV_SERIALIZE(locked_contributions)
        END_KV_SERIALIZE_MAP()
      };

      struct entry
      {
        std::string               service_node_pubkey;           // The public key of the Service Node.
        uint64_t                  registration_height;           // The height at which the registration for the Service Node arrived on the blockchain.
        uint64_t                  requested_unlock_height;       // The height at which contributions will be released and the Service Node expires. 0 if not requested yet.
        uint64_t                  last_reward_block_height;      // The last height at which this Service Node received a reward.
        uint32_t                  last_reward_transaction_index; // When multiple Service Nodes register on the same height, the order the transaction arrive dictate the order you receive rewards.
        uint64_t                  last_uptime_proof;             // The last time this Service Node's uptime proof was relayed by atleast 1 Service Node other than itself in unix epoch time.
        std::vector<contributor>  contributors;                  // Array of contributors, contributing to this Service Node.
        uint64_t                  total_contributed;             // The total amount of Loki in atomic units contributed to this Service Node.
        uint64_t                  total_reserved;                // The total amount of Loki in atomic units reserved in this Service Node.
        uint64_t                  staking_requirement;           // The staking requirement in atomic units that is required to be contributed to become a Service Node.
        uint64_t                  portions_for_operator;         // The operator percentage cut to take from each reward expressed in portions, see cryptonote_config.h's STAKING_PORTIONS.
        std::string               operator_address;              // The wallet address of the operator to which the operator cut of the staking reward is sent to.

        BEGIN_KV_SERIALIZE_MAP()
            KV_SERIALIZE(service_node_pubkey)
            KV_SERIALIZE(registration_height)
            KV_SERIALIZE(requested_unlock_height)
            KV_SERIALIZE(last_reward_block_height)
            KV_SERIALIZE(last_reward_transaction_index)
            KV_SERIALIZE(last_uptime_proof)
            KV_SERIALIZE(contributors)
            KV_SERIALIZE(total_contributed)
            KV_SERIALIZE(total_reserved)
            KV_SERIALIZE(staking_requirement)
            KV_SERIALIZE(portions_for_operator)
            KV_SERIALIZE(operator_address)
        END_KV_SERIALIZE_MAP()
      };

      std::vector<entry> service_node_states; // Array of service node registration information
      std::string status;                     // Generic RPC error code. "OK" is the success value.
      std::string as_json;                    // If `include_json` is set in the request, this contains the json representation of the `entry` data structure

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(service_node_states)
        KV_SERIALIZE(status)
        KV_SERIALIZE(as_json)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  // Example
  // curl -X POST http://127.0.0.1:38157/json_rpc -d '{"jsonrpc":"2.0","id":"0","method":"get_staking_requirement", "params": {“height”: 111111}}' -H 'Content-Type: application/json'
  // {
  //   "id": "0",
  //   "jsonrpc": "2.0",
  //   "result": {
  //     "staking_requirement": 100000000000,
  //     "status": "OK"
  //   }
  // }
  struct COMMAND_RPC_GET_STAKING_REQUIREMENT
  {
    struct request
    {
      uint64_t height; // The height to query the staking requirement for.
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t staking_requirement; // The staking requirement in Loki, in atomic units.
      std::string status;           // Generic RPC error code. "OK" is the success value.
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(staking_requirement)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_SERVICE_NODE_BLACKLISTED_KEY_IMAGES
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct entry
    {
      std::string key_image;  // The key image of the transaction that is blacklisted on the network.
      uint64_t unlock_height; // The height at which the key image is removed from the blacklist and becomes spendable.
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key_image)
        KV_SERIALIZE(unlock_height)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<entry> blacklist; // Array of blacklisted key images, i.e. unspendable transactions
      std::string status;           // Generic RPC error code. "OK" is the success value.
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(blacklist)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  LOKI_RPC_DOC_INTROSPECT
  struct COMMAND_RPC_GET_OUTPUT_BLACKLIST
  {
    struct request { BEGIN_KV_SERIALIZE_MAP() END_KV_SERIALIZE_MAP() };
    struct response
    {
      std::vector<uint64_t> blacklist; // (Developer): Array of indexes from the global output list, corresponding to blacklisted key images.
      std::string status;              // Generic RPC error code. "OK" is the success value.
      bool untrusted;                  // If the result is obtained using bootstrap mode, and therefore not trusted `true`, or otherwise `false`.
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(blacklist)
        KV_SERIALIZE(status)
        KV_SERIALIZE(untrusted)
      END_KV_SERIALIZE_MAP()
    };
  };
}
