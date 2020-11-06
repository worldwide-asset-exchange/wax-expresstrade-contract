#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/transaction.hpp>
#include <eosio/crypto.hpp>
#include <eosio/print.hpp>

#include <tuple>
#include <vector>
#include <string>
#include <json.hpp>

using json = nlohmann::json;
using namespace eosio;
using namespace std;

//#define EOSCHAIN
#define WAXCHAIN

const bool hasLogging = true;
const name SIMPLEASSETS_CONTRACT = "simpleassets"_n;

const name EOSIO_TOKEN       = "eosio.token"_n;
const name WAX_EXPRESS_TRADE = "barter111111"_n;

enum token_type { SA_NFT = 0, SA_FT = 1, TOKEN = 2 };

typedef uint64_t assettype_t;
typedef name	 author_t;

CONTRACT AffiliateBank : public contract {
public:
	using contract::contract;
	static uint64_t code_test;

	struct exchange_fees
	{
		name     exchange;
		uint16_t exchange_fee;
		uint64_t affiliate_id;
		uint16_t affiliate_fee;
	};

	struct asset_ex
	{
		name        author;
		asset       quantity;
		assettype_t assettype;
	};

	struct key_value
	{
		string key;
		string value;
	};

	ACTION modaffiliate(const uint64_t& affiliate_id, const name& exchange, const name& affiliate, const string& affiliate_name, const vector<key_value>& data);
	using modaffiliate_action = action_wrapper<"modaffiliate"_n, &AffiliateBank::modaffiliate>;

	ACTION modexchange(const name& exchange, const vector<key_value>& data);
	using modexchange_action = action_wrapper<"modexchange"_n, &AffiliateBank::modexchange>;

	ACTION addexchange(const name& exchange, const vector<key_value>& data);
	using addexchange_action = action_wrapper<"addexchange"_n, &AffiliateBank::addexchange>;

	ACTION addaffiliate(const name& exchange, const name& affiliate, const string& affiliate_name, const vector<key_value>& data);
	using addaffiliate_action = action_wrapper<"addaffiliate"_n, &AffiliateBank::addaffiliate>;

	ACTION withdraw(const uint64_t & affiliate_id, const name& affiliate, vector<asset_ex>& fts);
	using withdraw_action = action_wrapper<"withdraw"_n, &AffiliateBank::withdraw>;

	void receiveASSETF(name from, name to, name author, asset quantity, string memo);
	void receiveToken (name from, name to, asset quantity, string memo);
private:
	const string errorWAXTradeOnly = "Only Wax Express Trade can send transactions to Affiliate Bank";
	void depositFT(exchange_fees fee, asset_ex payment);
	AffiliateBank::exchange_fees parseMemo(const string& memo);
public:
	TABLE exchanges{
		name				exchange;
		vector<key_value>	data;

		auto primary_key() const {
			return exchange.value;
		}
	};
	typedef eosio::multi_index< "sexchanges"_n, exchanges > sexchanges;

	TABLE affiliates{
		uint64_t			id;
		name				exchange;
		name				affiliate;
		string				affiliate_name;
		vector<asset_ex>	balance;
		vector<key_value>	data;

		uint64_t by_exchange() const {
			return exchange.value;
		}

		auto primary_key() const {
			return id;
		}
	};
	typedef eosio::multi_index< "saffiliates"_n, affiliates ,
		eosio::indexed_by< "exchange"_n, eosio::const_mem_fun<affiliates, uint64_t, &affiliates::by_exchange>>
	> saffiliates;
private:
	saffiliates saffiliates_ = { _self, _self.value };
	sexchanges  sexchanges_ = { _self, _self.value };
};

extern "C"
void apply(uint64_t receiver, uint64_t code, uint64_t action)
{
	AffiliateBank::code_test = code;

	if (code == SIMPLEASSETS_CONTRACT.value && action == "transferf"_n.value) {
		eosio::execute_action(eosio::name(receiver), eosio::name(code), &AffiliateBank::receiveASSETF);
	}
	else if (action == "transfer"_n.value) {
		eosio::execute_action(eosio::name(receiver), eosio::name(code), &AffiliateBank::receiveToken);
	}
	else if (code == receiver) {

		switch (action) {
#ifdef DEBUG
			EOSIO_DISPATCH_HELPER(AffiliateBank, (addexchange)(addaffiliate)(withdraw)(modexchange)(modaffiliate))
#else
			EOSIO_DISPATCH_HELPER(AffiliateBank, (addexchange)(addaffiliate)(withdraw)(modexchange)(modaffiliate))
#endif
		}
	}
}

uint64_t AffiliateBank::code_test = 0;