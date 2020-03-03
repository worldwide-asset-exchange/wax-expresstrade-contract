
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/transaction.hpp>
#include <eosio/crypto.hpp>
#include <eosio/print.hpp>
#include <map>
#include <tuple>
#include <vector>
#include <string>
#include <json.hpp>

using json = nlohmann::json;
using namespace eosio;
using namespace std;

//#define DEBUG
//#define EOS_CHAIN
#define WAX_CHAIN
bool hasLogging = true;

const name SIMPLEASSETS_CONTRACT = "simpleassets"_n; 
const name EOSIO_TOKEN           = "eosio.token"_n; 

enum token_type {SA_NFT = 0, SA_FT = 1, TOKEN = 2};

typedef uint64_t box_id_t;
typedef uint64_t object_id_t;
typedef uint64_t assettype_t;
typedef uint64_t nft_id_t;
typedef uint64_t inventory_id_t;
typedef string   key_t;
typedef string   operation_t;
typedef string   value_t;
typedef uint64_t amount_t;

CONTRACT Barter : public contract{
   public:
	  using contract::contract;

	public:
	  static uint64_t code_test;
	  struct exchange_fees {
		  name     exchange;
		  uint32_t exchange_fee;
		  uint64_t affiliate_id;
		  uint32_t affiliate_fee;
	  };

	  ACTION withdraw(name owner, vector<nft_id_t>& nfts, vector<tuple<name, asset, assettype_t>>& fts);
	  using withdraw_action = action_wrapper<"withdraw"_n, &Barter::withdraw>;

	  ACTION createprop(name owner, vector<nft_id_t>& nfts, vector<tuple<name, asset, assettype_t>>& fts, vector<tuple<box_id_t, object_id_t, key_t, operation_t, value_t>>& conditions, exchange_fees fees, uint64_t topropid, name account_to, uint64_t datestart, uint64_t dateexpire, bool auto_accept, string memo);
	  using createprop_action = action_wrapper<"createprop"_n, &Barter::createprop>;

	  ACTION acceptprop(name owner, uint64_t proposal_id, uint64_t box_id);
	  using acceptprop_action = action_wrapper<"acceptprop"_n, &Barter::acceptprop>;
 
	  ACTION cancelprop(name owner, uint64_t proposal_id);
	  using cancelprop_action = action_wrapper<"cancelprop"_n, &Barter::cancelprop>;

	  ACTION rejectoffer(name owner, uint64_t offer_id);
	  using rejectoffer_action = action_wrapper<"rejectoffer"_n, &Barter::rejectoffer>;
  
	  ACTION cancelwish(name owner, uint64_t wish_id);
	  using cancelwish_action = action_wrapper<"cancelwish"_n, &Barter::cancelwish>;
	  
	  ACTION createwish(name owner, vector<tuple<key_t, operation_t, value_t>>& conditions);
	  using createwish_action = action_wrapper<"createwish"_n, &Barter::createwish>;

	  ACTION addblacklist(name blacklisted_author, string memo);
	  using addblacklist_action = action_wrapper<"addblacklist"_n, &Barter::addblacklist>;

	  ACTION delblacklist(name blacklisted_author);
	  using delblacklist_action = action_wrapper<"delblacklist"_n, &Barter::delblacklist>;

	  ACTION getbalance(name owner, name author, string sym);
	  using getbalance_action = action_wrapper<"getbalance"_n, &Barter::getbalance>;

	  ACTION getversion();
	  using getversion_action = action_wrapper<"getversion"_n, &Barter::getversion>;

	  void receiveToken(name from, name to, asset quantity, string memo);
	  void receiveASSET (name from, name to, vector<uint64_t>& assetids, string memo);
	  void receiveASSETF(name from, name to, name author, asset quantity, string memo);
#ifdef DEBUG
public:
	ACTION tstcondition(const vector<tuple<box_id_t, object_id_t, key_t, operation_t, value_t>>& conditions);
	using tstcondition_action = action_wrapper<"tstcondition"_n, &Barter::tstcondition>;

	ACTION eraseallprop();
	using eraseallprop_action = action_wrapper<"eraseallprop"_n, &Barter::eraseallprop>;

	ACTION changetype(name owner, uint64_t inventory_id, uint64_t assettype);
	using changetype_action = action_wrapper<"changetype"_n, &Barter::changetype>;

	ACTION delinventory(name owner, uint64_t inventory_id);
	using delinventory_action = action_wrapper<"delinventory"_n, &Barter::delinventory>;

	ACTION delproposal(name owner, uint64_t inventory_id);
	using delproposal_action = action_wrapper<"delproposal"_n, &Barter::delproposal>;

	ACTION delcondition(name owner, uint64_t conditionid);
	using delcondition_action = action_wrapper<"delcondition"_n, &Barter::delcondition>;

	ACTION datamatch(uint64_t inventory_id, tuple<key_t, operation_t, value_t>& condition);
	using datamatch_action = action_wrapper<"datamatch"_n, &Barter::datamatch>;

	ACTION checkaccept(name owner, uint64_t proposal_id, uint64_t box_id);
	using checkaccept_action = action_wrapper<"checkaccept"_n, &Barter::checkaccept>;

	ACTION testgetvalue(string filter, string imdata);
	using testgetvalue_action = action_wrapper<"testgetvalue"_n, &Barter::testgetvalue>;

	ACTION testisint(const string& value);
	using testisint_action = action_wrapper<"testisint"_n, &Barter::testisint>;

	ACTION tstwithdraw(name owner, name asset_author, asset withraw_asset, uint64_t assettype);
	using tstwithdraw_action = action_wrapper<"tstwithdraw"_n, &Barter::tstwithdraw>;
#endif 
public:

	/* Inventory // all the stuff in the WAX WET system and who owns it
		uint64_t : id * // unique id for every NFT or FT. (If two different owners have the same FT (like PGL), it get different ids.
		name : owner * // wax account
		uint64_t : aid * // Simple Asset ID
		uint : assetype // SA NFT, SA FT, or a # for every unique non-SA tokens
		asset : quantity // for SA FTs, or non-SA tokens
		vector <uint64_t> : inproposals // array of all existing proposals which contain this asset (Needed for easy cancellation of offers if asset is traded elsewhere.)
		name : author * ? // SA author
		name : cat * ? // SA category
		string : mdata // SA mdata - for faster reference (tentative)
		// mdata can theoretically get updated by the game
		// at any time, but it's unlikely and also difficult to
		// locate an asset whose owner isn't engaged in the game.
		string : idata // SA idata - for faster reference (tentative) */

	TABLE inventory{
		uint64_t                id;         // unique id for every NFT or FT. (If two different owners have the same FT (like PGL), it get different ids.
		name                    owner;      // wax account
		uint64_t                aid;        // Simple Asset ID
		uint64_t                assettype;  // SA NFT, SA FT, or a # for every unique non-SA tokens
		asset                   quantity;   // for SA FTs, or non-SA tokens
		vector<uint64_t>        inproposal; // array of all existing proposals which contain this asset (Needed for easy cancellation of offers if asset is traded elsewhere.)
		name                    author;     // SA author
		name                    category;   // SA category
		string                  idata;      // immutable data
		string                  mdata;      // mutable data

		auto primary_key() const {
			return id;
		}
		uint64_t by_author() const {
			return author.value;
		}
		uint64_t by_owner() const {
			return owner.value;
		}
		uint64_t by_aid() const {
			return aid;
		}
	};
	typedef eosio::multi_index< "sinventory"_n, inventory,
		eosio::indexed_by< "author"_n, eosio::const_mem_fun<inventory, uint64_t, &inventory::by_author> >,
		eosio::indexed_by< "owner"_n,  eosio::const_mem_fun<inventory, uint64_t, &inventory::by_owner> >,
		eosio::indexed_by< "aid"_n,    eosio::const_mem_fun<inventory, uint64_t, &inventory::by_aid> >
	> sinventory;

	/*
		conditions
		uint64_t: id *     // unique id for every condition
		name: owner*       // account making the condition
		uint64_t: boxid;   // id for every group of objects in proposal
		vector<tuple<key,op, val>> : aconditions // auto-accept conditions (see below)
	*/

	TABLE conditions{
		uint64_t				id;
		name					owner;      // account making the proposal and condition
		uint64_t				proposalid; // unique id for proposal that connected this condition 
		uint64_t				boxid;      // id for every group of objects in proposal 
		uint64_t				objectid;   // id for every object in boix for proposal 
		vector<tuple<key_t, operation_t, value_t> >     aconditions; // auto-accept conditions (see below). < key , operation(= != > <), value >
		auto primary_key() const {
			return id;
		}
		uint64_t by_proposal() const {
			return proposalid;
		}
	};
	typedef eosio::multi_index< "sconditions"_n, conditions,
		eosio::indexed_by< "proposalid"_n, eosio::const_mem_fun<conditions, uint64_t, &conditions::by_proposal> >
	> sconditions;

	/*
		tproposals
		uint64_t: id *              // unique id for every proposal
		uint8_t or uint62_t: aaid*? // if we are auto-matching on-chain then we  may need optimizied way to quick identify proposals with auto-accept conditions
		name: owner*                // account making the proposal
		vector <uint64_t>: nfts     // all SA NFTs in the offering
		vector <name, asset>: fts   // all FTs (whether SA or not)
		vector<tuple<key,op, val>> : aconditions // auto-accept conditions (see below)
		exchange_fees fees;			// fee for exchange
		name: toaccount*			// Wax account is offer is made exclusively to another user
		uint64_t: datestart			// (optional) proposal activation DTG
		uint64_t: dateexpire		// (optional) proposal expiration DTG
		uint64_t: topropid*			// An "offer" made to a specific existing proposals
		string: memo				// with a character limit of 256?
	*/
	TABLE tproposals {
		uint64_t				id;          // unique id for every proposal
		uint64_t				aaid;        // if we are auto-matching on-chain then we  may need optimizied way to quick identify proposals with auto-accept conditions
		name					owner;       // account making the proposal
		vector<uint64_t>		nfts;        // all SA NFTs in the offering
		vector<tuple<name, asset, assettype_t>> fts;      // all FTs (whether SA or not)
		exchange_fees			fees;        // fee for exchange
		name					toaccount;   // Wax account is offer is made exclusively to another user
		uint64_t				datestart;   // (optional) proposal activation DTG
		uint64_t				dateexpire;  // (optional) proposal expiration DTG
		uint64_t				topropid;    // An "offer" made to a specific existing proposals
		bool					auto_accept; // is proposal allow auto accept 
		string					memo;        // with a character limit of 256? 

		auto primary_key() const {
			return id;
		}

		uint64_t by_owner() const {
			return owner.value;
		}

		uint64_t by_toaccount() const {
			return toaccount.value;
		}

		uint64_t by_topropid() const {
			return topropid;
		}
	};
	typedef eosio::multi_index< "stproposals"_n, tproposals,
		eosio::indexed_by< "owner"_n,     eosio::const_mem_fun<tproposals, uint64_t, &tproposals::by_owner> >,
		eosio::indexed_by< "toaccount"_n, eosio::const_mem_fun<tproposals, uint64_t, &tproposals::by_toaccount> >,
		eosio::indexed_by< "topropid"_n,  eosio::const_mem_fun<tproposals, uint64_t, &tproposals::by_topropid> >
	> stproposals;


	/*
		wish
		uint64_t: id *     // unique id for every wish item
		name: owner*       // account making the wish
		vector<tuple<key,op, val>> : conditions // auto-accept conditions (see below)
	*/

	TABLE wish {
		uint64_t				id;
		name					owner;
		vector<tuple<key_t, operation_t, value_t> >     conditions; // auto-accept conditions (see below). < key , operation(= != > <), value >

		auto primary_key() const {
			return id;
		}
		uint64_t by_owner() const {
			return owner.value;
		}
	};
	typedef eosio::multi_index< "swish"_n, wish,
		eosio::indexed_by< "owner"_n, eosio::const_mem_fun<wish, uint64_t, &wish::by_owner> >
	> swish;

	/*
		Author`s of SA tokens blacklist
		name: author*       // blacklisted author
	*/

	TABLE blacklist{
		name author;
		string memo;

		auto primary_key() const {
			return author.value;
		}
	};
	typedef eosio::multi_index< "sblacklist"_n, blacklist > sblacklist;

	TABLE exchange{
		uint64_t				id;
		uint128_t				eid;
		name					tcontract;
		asset					quantity;

		auto primary_key() const {
			return id;
		}

		uint128_t by_eid() const {
			return eid;
		}
	};
	typedef eosio::multi_index< "sexchange"_n, exchange,
		eosio::indexed_by< "eid"_n, eosio::const_mem_fun<exchange, uint128_t, &exchange::by_eid> >
	> sexchange;

	TABLE currency_stats{
		asset		supply;
		asset		max_supply;
		name		issuer;
		uint64_t 	id;
		bool		authorctrl;
		string		data;

		uint64_t primary_key()const {
			return supply.symbol.code().raw();
		}
	};
	typedef eosio::multi_index< "stat"_n, currency_stats > stats;

	TABLE account{
		uint64_t	id;
		uint64_t	author;
		asset		balance;

		uint64_t primary_key()const {
			return id;
		}
	};
	typedef eosio::multi_index< "accounts"_n, account > accounts;

	TABLE sasset{
		uint64_t	id;
		name		owner;
		name		author;
		name		category;
		string		idata; // immutable data
		string		mdata; // mutable data

		vector<sasset> container;
		vector<account> containerf;

		auto primary_key() const {
			return id;
		}

		uint64_t by_author() const {
			return author.value;
		}
	};
	typedef eosio::multi_index< "sassets"_n, sasset,
		eosio::indexed_by< "author"_n, eosio::const_mem_fun< sasset, uint64_t, &sasset::by_author > >
	> sassets;

private:
	const string warning_must_be_condition = "Must be at least one condition";
	const string withdraw_memo             = "WAXWETBarter assets withdraw";

private:
	sinventory sinventory_   = { _self, _self.value };
	sconditions sconditions_ = { _self, _self.value };
	stproposals stproposals_ = { _self, _self.value };
	swish swish_             = { _self, _self.value };
	sblacklist sblacklist_   = { _self, _self.value };
	
	vector<inventory_id_t> buyer_to_exchange;
	map<inventory_id_t, amount_t> asset_to_subtract;
	map<box_id_t, vector<tuple<key_t, operation_t, value_t>>> mapObjectsNFT;
	map<box_id_t, vector<tuple<key_t, operation_t, value_t>>> mapObjectsFT;

	void remove_all_proposals_for_inventory_item(const uint64_t& inventory_id);
	void remove_all_offers_for_proposal(const uint64_t& proposal_id);
	void remove_proposal_with_conditions(const uint64_t& proposal_id);
	void exchange_aftermatch(name owner, uint64_t proposal_id);
	void find_match_for_owner(const name& owner, const uint64_t& proposal_id, const uint64_t& box_id);
	bool ismatchNFT(const name& owner, const tuple<key_t, operation_t, value_t> &onecondition, const name& inventory_author, const unsigned& inventory_assettype, const name& inventory_category, const name& inventory_owner, const uint64_t& inventory_aid, const string& inventory_mdata, const string& inventory_idata);
	bool isdatamatch(const tuple<key_t, operation_t, value_t>& condition, const string& imdata);
	bool isinteger(const string &str);
	uint64_t findNFTfromConditions(name owner, const vector<tuple<key_t, operation_t, value_t> > & aconditions);
	void depositFT(name deposit_account, name author, asset quantity, uint64_t assettype);
	bool canwithdraw(name owner, name author, asset amount_to_withdraw, uint64_t assettype);
	void split(const string& subject, vector<string>& container);
	bool get_value(const string& filter, const string& imdata, string& result);
	void print_tuple(const tuple<key_t, operation_t, value_t> &onecondition);
	void create_conditions(const name& owner, const uint64_t& proposal_id, const vector<tuple<box_id_t, object_id_t, key_t, operation_t, value_t>>& conditions);
	uint64_t getid();
	string gettimeToWait(uint64_t time_in_seconds);

	void syntaxis_checker(const map < tuple < box_id_t, object_id_t>, vector<tuple<key_t, operation_t, value_t>>> & mapconditions);
	void check_duplication(const unsigned & count, const string &count_name, const unsigned &box_id, const unsigned& object_id);

	void check_key(const box_id_t & box_id, const object_id_t & object_id, const tuple<key_t, operation_t, value_t>& onecondition);
	void check_operation(const box_id_t & box_id, const object_id_t & object_id, const tuple<key_t, operation_t, value_t>& onecondition);
	void check_value(const box_id_t & box_id, const object_id_t & object_id, const tuple<key_t, operation_t, value_t>& onecondition);

	bool hasDublicatedNFT(const vector<uint64_t>& nfts, string& out_dublicated_nft);
	bool hasDublicatedFT(const vector<tuple<name, asset, assettype_t>>& fts, string& out_dublicated_ft);
	auto find_condition(const vector<tuple<key_t, operation_t, value_t>>& aconditions, const string& condition_key);

private:

#ifdef WAX_CHAIN
	const string SYMBOL_NAME = "WAX";
	const int PRECISION = 8;
#else
#ifdef EOS_CHAIN
	const string SYMBOL_NAME = "EOS";
	const int PRECISION = 4;
#endif
#endif

	asset asset_from_string(const string& s);
	const vector<string> explode(const string& s, const char& c);
};

extern "C"
void apply(uint64_t receiver, uint64_t code, uint64_t action)
{
	Barter::code_test = code;

	if (code == SIMPLEASSETS_CONTRACT.value && action == "transfer"_n.value) {
		eosio::execute_action(eosio::name(receiver), eosio::name(code), &Barter::receiveASSET);
	}
	else if (code == SIMPLEASSETS_CONTRACT.value && action == "transferf"_n.value) {
		eosio::execute_action(eosio::name(receiver), eosio::name(code), &Barter::receiveASSETF);
	}
	else if (action == "transfer"_n.value) {
		eosio::execute_action(eosio::name(receiver), eosio::name(code), &Barter::receiveToken);
	}
	else if (code == receiver) {

		switch (action) {
#ifdef DEBUG
			EOSIO_DISPATCH_HELPER(Barter, (createprop)(withdraw)(cancelprop)(acceptprop)(delcondition)(delproposal)(datamatch)(checkaccept)(rejectoffer)(delinventory)(testgetvalue)(getbalance)(createwish)(cancelwish)(testisint)(tstwithdraw)(getversion)(eraseallprop)(changetype)(delblacklist)(addblacklist)(tstcondition))
#else
			EOSIO_DISPATCH_HELPER(Barter, (createprop)(withdraw)(cancelprop)(acceptprop)(rejectoffer)(getbalance)(createwish)(cancelwish)(getversion)(delblacklist)(addblacklist))
#endif
		}
	}
}

uint64_t Barter::code_test = 0;