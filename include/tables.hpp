#pragma once

#include <eosio/eosio.hpp>
#include <map>
#include <tuple>
#include <vector>
#include <string>
#include <exchange_fees.hpp>
#include <date_range.hpp>

using namespace eosio;
using namespace std;

class tables
{
	/* Inventory // all the stuff in the WAX WET system and who owns it
	uint64_t : id * // unique id for every NFT or FT. (If two different owners have the same FT (like PGL), it get different ids.
	name : owner * // wax account
	uint64_t : aid * // Simple Asset ID
	uint : assetype // SA NFT, SA FT, or a # for every unique non-SA tokens
	asset : quantity // for SA FTs, or non-SA tokens
	vector <uint64_t> : inproposals // array of all existing proposals which contain this asset (Needed for easy cancellation of offers if asset is traded elsewhere.)
	name : author * ? // SA author
*/
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
		uint64_t by_aid() const {
			return aid;
		}
	};
	typedef eosio::multi_index< "sinventory"_n, inventory,
		eosio::indexed_by< "author"_n, eosio::const_mem_fun<inventory, uint64_t, &inventory::by_author> >,
		eosio::indexed_by< "owner"_n, eosio::const_mem_fun<inventory, uint64_t, &inventory::by_owner> >,
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
};
