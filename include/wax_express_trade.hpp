
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
#include <fee_processor.hpp>
#include <common.hpp>
#include <exchange_fees.hpp>
#include <syntax_checker.hpp>
#include <acceptor.hpp>
#include <tables.hpp>
#include <date_range.hpp>
#include <inventory_bank.hpp>

using json = nlohmann::json;
using namespace eosio;
using namespace std;

CONTRACT wax_express_trade : public contract{
   public:
	using contract::contract;

	public:
	  static uint64_t code_test;

	  ACTION depositprep(name owner, vector<nft_id_t>& nfts, vector<asset_ex>& fts);
	  using depositprep_action = action_wrapper<"depositprep"_n, &wax_express_trade::depositprep>;

	  ACTION withdraw(name owner, vector<nft_id_t>& nfts, vector<asset_ex>& fts);
	  using withdraw_action = action_wrapper<"withdraw"_n, &wax_express_trade::withdraw>;

	  ACTION createprop(name owner, const vector<nft_id_t>& nfts, const vector<asset_ex>& fts, const vector<tuple<box_id_t, object_id_t, condition>>& conditions, exchange_fees fees, name account_to, date_range daterange, bool auto_accept, string memo);
	  using createprop_action = action_wrapper<"createprop"_n, &wax_express_trade::createprop>;

	  ACTION cancelprop(name owner, uint64_t proposal_id);
	  using cancelprop_action = action_wrapper<"cancelprop"_n, &wax_express_trade::cancelprop>;

	  ACTION createoffer(name owner, const vector<nft_id_t>& nfts, const vector<asset_ex>& fts, proposal_id_t topropid, box_id_t box_id, date_range daterange, string memo);
	  using createoffer_action = action_wrapper<"createoffer"_n, &wax_express_trade::createoffer>;

	  ACTION acceptoffer(name owner, uint64_t offer_id);
	  using acceptoffer_action = action_wrapper<"acceptoffer"_n, &wax_express_trade::acceptoffer>;

	  ACTION rejectoffer(name owner, uint64_t offer_id);
	  using rejectoffer_action = action_wrapper<"rejectoffer"_n, &wax_express_trade::rejectoffer>;

	  ACTION requestgift(name owner, const vector<tuple<box_id_t, object_id_t, condition>>& conditions, name account_to, date_range daterange, string memo);
	  using requestgift_action = action_wrapper<"requestgift"_n, &wax_express_trade::requestgift>;

	  ACTION creategift(name owner, const vector<nft_id_t>& nfts, const vector<asset_ex>& fts, name account_to, date_range daterange, string memo);
	  using creategift_action = action_wrapper<"creategift"_n, &wax_express_trade::creategift>;

	  ACTION acceptgift(name owner, uint64_t gift_id);
	  using acceptgift_action = action_wrapper<"acceptgift"_n, &wax_express_trade::acceptgift>;

	  ACTION sendgift(name owner, const vector<nft_id_t>& nfts, const vector<asset_ex>& fts, uint64_t gift_id, uint64_t box_id);
	  using sendgift_action = action_wrapper<"sendgift"_n, &wax_express_trade::sendgift>;

	  ACTION cancelwish(name owner, uint64_t wish_id);
	  using cancelwish_action = action_wrapper<"cancelwish"_n, &wax_express_trade::cancelwish>;

	  ACTION createwish(name owner, vector<condition>& conditions);
	  using createwish_action = action_wrapper<"createwish"_n, &wax_express_trade::createwish>;

	  ACTION addblacklist(name blacklisted_author, string memo);
	  using addblacklist_action = action_wrapper<"addblacklist"_n, &wax_express_trade::addblacklist>;

	  ACTION delblacklist(name blacklisted_author);
	  using delblacklist_action = action_wrapper<"delblacklist"_n, &wax_express_trade::delblacklist>;

	  ACTION getbalance(name owner, name author, string sym);
	  using getbalance_action = action_wrapper<"getbalance"_n, &wax_express_trade::getbalance>;

	  ACTION getversion();
	  using getversion_action = action_wrapper<"getversion"_n, &wax_express_trade::getversion>;

	  ACTION delinventory(uint64_t inventory_id);
	  using delinventory_action = action_wrapper<"delinventory"_n, &wax_express_trade::delinventory>;

	  void receiveToken(name from, name to, asset quantity, string memo);
	  void receiveASSET(name from, name to, vector<uint64_t>& assetids, string memo);
	  void receiveASSETF(name from, name to, name author, asset quantity, string memo);
#ifdef DEBUG
public:
	ACTION tstfee(name nft_author, exchange_fees fee, asset_ex payment_ft_from_proposal);
	using tstfee_action = action_wrapper<"tstfee"_n, &wax_express_trade::tstfee>;

	ACTION tstcondition(const vector<tuple<box_id_t, object_id_t, condition>>& conditions, bool isoffer);
	using tstcondition_action = action_wrapper<"tstcondition"_n, &wax_express_trade::tstcondition>;

	ACTION eraseallprop();
	using eraseallprop_action = action_wrapper<"eraseallprop"_n, &wax_express_trade::eraseallprop>;

	ACTION changetype(name owner, uint64_t inventory_id, uint64_t assettype);
	using changetype_action = action_wrapper<"changetype"_n, &wax_express_trade::changetype>;

	ACTION delproposal(name owner, uint64_t inventory_id);
	using delproposal_action = action_wrapper<"delproposal"_n, &wax_express_trade::delproposal>;

	ACTION delcondition(name owner, uint64_t conditionid);
	using delcondition_action = action_wrapper<"delcondition"_n, &wax_express_trade::delcondition>;

	ACTION datamatch(uint64_t inventory_id, const condition& condition);
	using datamatch_action = action_wrapper<"datamatch"_n, &wax_express_trade::datamatch>;

	ACTION testgetvalue(string filter, string imdata);
	using testgetvalue_action = action_wrapper<"testgetvalue"_n, &wax_express_trade::testgetvalue>;

	ACTION testisint(const string& value);
	using testisint_action = action_wrapper<"testisint"_n, &wax_express_trade::testisint>;

	ACTION tstwithdraw(name owner, name asset_author, asset withraw_asset, uint64_t assettype);
	using tstwithdraw_action = action_wrapper<"tstwithdraw"_n, &wax_express_trade::tstwithdraw>;

	ACTION tstautfee(name author);
	using tstautfee_action = action_wrapper<"tstautfee"_n, &wax_express_trade::tstautfee>;

	ACTION isgift(uint64_t proposal_id);
	using isgift_action = action_wrapper<"isgift"_n, &wax_express_trade::isgift>;

#endif 

private:
	const string must_be_at_least_one = "Must be at least";
	const string warning_one_condition = " one condition ";
	const string errorEmptyFT_NFT = " one fungible or non-fungible token";
	const string withdraw_memo = "Express Trade assets withdraw";

private:
	tables::sinventory sinventory_ = { _self, _self.value };
	tables::sconditions sconditions_ = { _self, _self.value };
	tables::stproposals stproposals_ = { _self, _self.value };
	tables::swish swish_ = { _self, _self.value };
	tables::sblacklist sblacklist_ = { _self, _self.value };
	tables::sassets assets_ = { SIMPLEASSETS_CONTRACT, _self.value };

	syntax_checker checker;
	fee_processor fee_processor;
	inventory_bank inventory_bank = { sinventory_, stproposals_, get_self() };
	acceptor acceptor_ = { get_self(), sinventory_ , stproposals_, sconditions_, assets_, fee_processor, inventory_bank };

	void create_conditions(const name& owner, const uint64_t& proposal_id, const map < tuple < box_id_t, object_id_t >, vector<condition>>& mapconditions);
	bool try_accept_proposal(name owner, proposal_id_t topropid, uint64_t box_id, const vector<nft_id_t>& nfts, const vector<asset_ex>& fts);
	void create_proposal(name owner, const vector<nft_id_t>& nfts, const vector<asset_ex>& fts, const vector<tuple<box_id_t, object_id_t, condition>>& conditions, exchange_fees fees, name account_to, proposal_id_t topropid, date_range daterange, bool auto_accept, string memo);
	void accept_offer(name owner, uint64_t offer_id);
	void accept_gift(name owner, uint64_t gift_id);

	uint64_t getid();

	public:

	TABLE inventory{
		uint64_t				id;         // unique id for every NFT or FT. (If two different owners have the same FT (like PGL), it get different ids.
		name					owner;      // wax account
		uint64_t				aid;        // Simple Asset ID
		assettype_t				assettype;  // SA NFT, SA FT, or a # for every unique non-SA tokens
		asset					quantity;   // for SA FTs, or non-SA tokens
		vector<proposal_id_t>	inproposal; // array of all existing proposals which contain this asset (Needed for easy cancellation of offers if asset is traded elsewhere.)
		name					author;     // SA author

		auto primary_key() const {
			return id;
		}
		uint64_t by_author() const {
			return author.value;
		}
		uint64_t by_owner() const {
			return owner.value;
		}
		uint128_t by_owner2() const {
			return ((uint128_t)owner.value << 64) + id;
		}
		uint64_t by_aid() const {
			return aid;
		}
	};
	typedef eosio::multi_index< "sinventory"_n, inventory,
		eosio::indexed_by< "author"_n, eosio::const_mem_fun<inventory, uint64_t, &inventory::by_author> >,
		eosio::indexed_by< "owner"_n, eosio::const_mem_fun<inventory, uint64_t, &inventory::by_owner> >,
		eosio::indexed_by< "owner2"_n, eosio::const_mem_fun<inventory, uint128_t, &inventory::by_owner2> >,
		eosio::indexed_by< "aid"_n, eosio::const_mem_fun<inventory, uint64_t, &inventory::by_aid> >
	> sinventory;

	TABLE conditions{
		uint64_t				id;
		name					owner;      // account making the proposal and condition
		uint64_t				proposalid; // unique id for proposal that connected this condition 
		uint64_t				boxid;      // id for every group of objects in proposal 
		uint64_t				objectid;   // id for every object in boix for proposal 
		vector<condition>		aconditions; // auto-accept conditions (see below). < key , operation(= != > <), value >
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
		date_range	daterange;		// proposal activation DTG
		uint64_t: topropid*			// An "offer" made to a specific existing proposals
		string: memo				// with a character limit of 256?
	*/
	TABLE tproposals{
		proposal_id_t			id;          // unique id for every proposal
		uint64_t				aaid;        // if we are auto-matching on-chain then we  may need optimizied way to quick identify proposals with auto-accept conditions
		name					owner;       // account making the proposal
		vector<nft_id_t>		nfts;        // all SA NFTs in the offering
		vector<asset_ex>		fts;         // all FTs (whether SA or not)
		exchange_fees			fees;        // fee for exchange
		name					toaccount;   // Wax account is offer is made exclusively to another user
		date_range				daterange;   // proposal activation DTG
		proposal_id_t			topropid;    // An "offer" made to a specific existing proposals
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
		eosio::indexed_by< "owner"_n, eosio::const_mem_fun<tproposals, uint64_t, &tproposals::by_owner> >,
		eosio::indexed_by< "toaccount"_n, eosio::const_mem_fun<tproposals, uint64_t, &tproposals::by_toaccount> >,
		eosio::indexed_by< "topropid"_n, eosio::const_mem_fun<tproposals, uint64_t, &tproposals::by_topropid> >
	> stproposals;

	/*
		wish
		uint64_t: id *     // unique id for every wish item
		name: owner*       // account making the wish
		vector<tuple<key,op, val>> : conditions // auto-accept conditions (see below)
	*/

	TABLE wish{
		uint64_t				id;
		name					owner;
		vector<condition>		conditions; // auto-accept conditions (see below). < key , operation(= != > <), value >

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

	/*
	* global singelton table, used for id building. Scope: self
	*/
	TABLE global{
		global() {}
		uint64_t lastid = 1000000000;
		EOSLIB_SERIALIZE(global, (lastid))
	};

	typedef eosio::singleton< "global"_n, global > conf; /// singleton
	global _cstate; /// global state

};

extern "C"
void apply(uint64_t receiver, uint64_t code, uint64_t action)
{
	wax_express_trade::code_test = code;

	if (code == SIMPLEASSETS_CONTRACT.value && action == "transfer"_n.value) {
		eosio::execute_action(eosio::name(receiver), eosio::name(code), &wax_express_trade::receiveASSET);
	}
	else if (code == SIMPLEASSETS_CONTRACT.value && action == "transferf"_n.value) {
		eosio::execute_action(eosio::name(receiver), eosio::name(code), &wax_express_trade::receiveASSETF);
	}
	else if (action == "transfer"_n.value) {
		eosio::execute_action(eosio::name(receiver), eosio::name(code), &wax_express_trade::receiveToken);
	}
	else if (code == receiver) {

		switch (action) {
#ifdef DEBUG
			EOSIO_DISPATCH_HELPER(wax_express_trade, (createprop)(createoffer)(withdraw)(cancelprop)(acceptoffer)(delcondition)(delproposal)(datamatch)(rejectoffer)(delinventory)(testgetvalue)(getbalance)(createwish)(cancelwish)(testisint)(tstwithdraw)(getversion)(eraseallprop)(changetype)(delblacklist)(addblacklist)(tstcondition)(tstfee)(depositprep)(tstautfee)(acceptgift)(creategift)(requestgift)(sendgift)(isgift))
#else
			EOSIO_DISPATCH_HELPER(wax_express_trade, (createprop)(createoffer)(withdraw)(cancelprop)(acceptoffer)(rejectoffer)(getbalance)(createwish)(cancelwish)(getversion)(delblacklist)(addblacklist)(depositprep)(acceptgift)(creategift)(requestgift)(sendgift)(delinventory))
#endif
		}
	}
}

uint64_t wax_express_trade::code_test = 0;