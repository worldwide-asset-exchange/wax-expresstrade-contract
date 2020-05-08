#pragma once
#include <eosio/eosio.hpp>
#include <string>
#include <tables.hpp>
#include <fee_processor.hpp>
#include <inventory_bank.hpp>
#include <json.hpp>

using json = nlohmann::json;
using namespace eosio;
using namespace std;

struct acceptor
{
	tables::sinventory&		sinventory_;
	tables::stproposals&	stproposals_;
	tables::sconditions&	sconditions_;
	tables::sassets&		assets_;
	inventory_bank&			inventory_bank_;
	fee_processor&			fee_processor_;
	name					self_;
	proposal_id_t			proposal_id;

	vector<inventory_id_t>				matched_nfts;
	map<object_id_t, vector<condition>>	mapConditionObjectsNFT;
	map<object_id_t, vector<condition>>	mapConditionObjectsFT;

	vector<inventory_id_t> processed_nfts_inventory_ids;
	vector<inventory_id_t> processed_fts_inventory_ids;

public:
	acceptor(name self, tables::sinventory& inventory, tables::stproposals& proposals, tables::sconditions& conditions, tables::sassets& assets, fee_processor& feeprocessor, inventory_bank &inventorybank)
		: self_(self), sinventory_(inventory), stproposals_(proposals), sconditions_(conditions), assets_(assets), fee_processor_(feeprocessor), inventory_bank_(inventorybank)
	{

	}

	name get_self()
	{
		return self_;
	}

	void clean_proposals_offers_conditions()
	{
		check(!(proposal_id == 0), "Internal error. proposal id = 0");

		// remove all proposals and offers that connected with fts
		clean_proposal(proposal_id);

		// remove all inproposal for inventory item and remove all offers 
		for (auto items = processed_nfts_inventory_ids.begin(); items != processed_nfts_inventory_ids.end(); items++)
		{
			remove_all_proposals_for_inventory_item(*items);
		}
	}

	void move_items(name payer, name initial_owner, name new_owner, const vector<nft_id_t>& nfts, const vector<asset_ex>& fts, exchange_fees fees, uint64_t proposal_id_seller)
	{
		proposal_id = proposal_id_seller != 0 ? proposal_id_seller : proposal_id;

		const auto& inventory_byowner = sinventory_.template get_index<"owner"_n>();

		bool item_exist = false;
		optional<name> nft_author = std::nullopt;

		// transfering gift no need to pay for it 
		if (proposal_id_seller != 0)
		{
			nft_author = get_nft_author(proposal_id_seller);
		}

		for (auto itr_ft = fts.begin(); itr_ft != fts.end(); itr_ft++)
		{
			if (hasLogging) {
				print("\n Asse1 from owner = ", initial_owner.to_string(), " author = ", itr_ft->author.to_string() + " symbol: ", itr_ft->quantity.to_string(), " assettype = ", to_string(itr_ft->assettype));
			}

			item_exist = false;

			// loop over buyer inventory and check conditions for each item
			for (auto itr_inventory_byowner = inventory_byowner.find(initial_owner.value); itr_inventory_byowner != inventory_byowner.end(); itr_inventory_byowner++)
			{
				if (initial_owner.value != itr_inventory_byowner->owner.value) {
					break;
				}

				if (hasLogging) {

					print("\n fts.size(): ", fts.size());
					print("\n itr_inventory_byowner->author ", itr_inventory_byowner->author);
					print("\n itr_inventory_byowner->quantity ", itr_inventory_byowner->quantity);
					print("\n itr_inventory_byowner->assettype ", itr_inventory_byowner->assettype);
				}

				if (itr_inventory_byowner->author == itr_ft->author && itr_inventory_byowner->quantity.symbol.code() == itr_ft->quantity.symbol.code() && itr_inventory_byowner->assettype == itr_ft->assettype)
				{
					check(!(itr_inventory_byowner->quantity.amount < itr_ft->quantity.amount), "Not enough amount for fungible token of author: " + itr_ft->author.to_string() + " asset: " + itr_ft->quantity.to_string() + " . Buyer " + itr_inventory_byowner->owner.to_string() + " balance: " + itr_inventory_byowner->quantity.to_string());

					check(!(itr_inventory_byowner->quantity.symbol.precision() != itr_ft->quantity.symbol.precision()),
						"Wrong precision for ft parameter. Inventory id: " + to_string(itr_inventory_byowner->id) + ". Symbol: " + itr_inventory_byowner->quantity.symbol.code().to_string() +
						" has precision " + to_string(itr_inventory_byowner->quantity.symbol.precision()) + ". You entered precision: " +
						to_string(itr_ft->quantity.symbol.precision()));

					item_exist = true;

					const auto& itr_inventory = sinventory_.require_find(itr_inventory_byowner->id, ((string)("Inventory id = " + to_string(itr_inventory_byowner->id) + " does not exist ")).c_str());

					// subtract from initial owner inventory 
					sinventory_.modify(itr_inventory, payer, [&](auto& g) {
						g.quantity -= itr_ft->quantity;
					});

					if (hasLogging) {
						print("\n itr_buyer_inventory->quantity: ", itr_inventory_byowner->quantity);
						print("\n move to seller account that was subtracted from buyer");
					}

					asset_ex new_account_payment = *itr_ft;
					// subtruct here from asset payment to author and exchange fee. Exchange fee include affiliates fee
					if (nft_author != std::nullopt)
					{
						if (hasLogging) {
							print("\n new_account_payment: author =  ", new_account_payment.author, " new_account_payment.quantity =  ", new_account_payment.quantity, " itr_ft->assettype = ", itr_ft->assettype);
						}

						fee_processor_.start(get_self(), *nft_author, fees, new_account_payment);

						// payment asset to seller with subtructed author and exchange fee. Exchange fee include affiliates fee
						new_account_payment.quantity = fee_processor_.getBuyerPart().quantity;

						if (hasLogging) {
							print("\n fee_processor_.getBuyerPart().quantity:", fee_processor_.getBuyerPart().quantity, " amount: ", fee_processor_.getBuyerPart().quantity.amount, " precesion: ", fee_processor_.getBuyerPart().quantity.symbol.precision());
						}

#ifdef AFFILIATEBANK
						// deposit exchange`s fee
						if (const auto exchange_fee = fee_processor_.getExchangeFee(); exchange_fee.quantity.amount > 0)
						{
							inventory_bank_.depositFT(fees.exchange, exchange_fee.author, exchange_fee.quantity, exchange_fee.assettype);
						}
#endif

						const auto author_fee = fee_processor_.getAuthorFee();

						if (hasLogging) {
							print("\n deposit author`s fee");
						}

						// deposit author`s fee
						if (author_fee.quantity.amount > 0)
						{
							inventory_bank_.depositFT(*nft_author, author_fee.author, author_fee.quantity, author_fee.assettype);
						}

						if (hasLogging) {
							print("\n deposit to author account. author_fee.quantity: ", author_fee.quantity);
							print("\n author: ", nft_author->to_string());
						}
					}

					// move to new account that was subtracted from initial account
					inventory_bank_.depositFT(new_owner, new_account_payment.author, new_account_payment.quantity, new_account_payment.assettype);
				}
			}

			if (hasLogging) {
				print("\n Asset2 from owner = " + initial_owner.to_string() + " author = " + itr_ft->author.to_string() + " symbol: " + itr_ft->quantity.to_string() + " assettype = " + to_string(itr_ft->assettype));
				print("\n item_exist: ", item_exist);
			}

			check(!(item_exist == false), "Asset from owner = " + initial_owner.to_string() + " does not exist in inventory. author = " + itr_ft->author.to_string() + " symbol: " + itr_ft->quantity.to_string() + " assettype = " + to_string(itr_ft->assettype));
		}

		for (auto itr_nft = nfts.begin(); itr_nft != nfts.end(); itr_nft++)
		{
			for (auto itr_inventory_byowner = inventory_byowner.find(initial_owner.value); itr_inventory_byowner != inventory_byowner.end(); itr_inventory_byowner++)
			{
				if (initial_owner.value != itr_inventory_byowner->owner.value) {
					break;
				}

				if (itr_inventory_byowner->assettype == SA_NFT && itr_inventory_byowner->aid == *itr_nft)
				{
					const auto& itr_inventory = sinventory_.require_find(itr_inventory_byowner->id, ((string)("Inventory id = " + to_string(itr_inventory_byowner->id) + " does not exist ")).c_str());
					item_exist = true;

					processed_nfts_inventory_ids.push_back(itr_inventory->id);

					// move initial owner items to new owner repository
					sinventory_.modify(itr_inventory, payer, [&](auto& s) {
						s.owner = new_owner;
					});
				}
			}

			check(!(item_exist == false), "Asset from owner = " + initial_owner.to_string() + " does not exist in inventory. aid = " + to_string(*itr_nft) + "  ");
		}

	}

	bool is_proposal_gift(const uint64_t& proposal_id)
	{
		const auto itrProposal = stproposals_.require_find(proposal_id, string("Proposal id = " + to_string(proposal_id) + " does not exist").c_str());

		auto proposalid_index = sconditions_.template get_index<"proposalid"_n>();
		auto itr_one_object_conditions = proposalid_index.find(proposal_id);

		const auto isEmptyConditionList = (itr_one_object_conditions == proposalid_index.end()) || (itr_one_object_conditions != proposalid_index.end() && proposal_id != itr_one_object_conditions->proposalid);
		const auto isoffer = itrProposal->topropid != 0;

		// if not offer and condition list is empty
		return (isEmptyConditionList && !isoffer);
	}

	void separate_conditions_to_NFTs_FTs(const uint64_t& proposal_id, const uint64_t& box_id)
	{
		auto proposalid_index = sconditions_.template get_index<"proposalid"_n>();
		auto is_box_exist = false;

		for (auto itr_one_object_conditions = proposalid_index.find(proposal_id); itr_one_object_conditions != proposalid_index.end(); itr_one_object_conditions++)
		{
			if (proposal_id != itr_one_object_conditions->proposalid) {
				break;
			}

			if (itr_one_object_conditions->boxid == box_id)
			{
				is_box_exist = true; // box_id from parameter exist in proposal

				const auto assettype_condition = find_condition(itr_one_object_conditions->aconditions, "assettype");

				check(!(assettype_condition == std::nullopt), "assettype does not exist in conditions of objectid: " + to_string(itr_one_object_conditions->objectid));

				const auto assettype = atoi(assettype_condition->value.c_str());

				if (assettype == SA_FT || assettype == TOKEN)
				{
					for (auto it = itr_one_object_conditions->aconditions.begin(); it != itr_one_object_conditions->aconditions.end(); it++)
					{
						mapConditionObjectsFT[itr_one_object_conditions->objectid].push_back(*it);
					}
				}
				else if (assettype == SA_NFT)
				{
					for (auto it = itr_one_object_conditions->aconditions.begin(); it != itr_one_object_conditions->aconditions.end(); it++)
					{
						mapConditionObjectsNFT[itr_one_object_conditions->objectid].push_back(*it);
					}
				}
				else
				{
					check(false, "Condition does not have assettype");
				}
			}
		}

		check(!(is_box_exist == false), "Parameter box_id: " + to_string(box_id) + " does not exist for proposal id: " + to_string(proposal_id));
	}

	match match_FTsConditions_with_FTs(name owner, const vector<asset_ex>& fts)
	{
		match matchFT = no_need_match;

		const auto isEmptyConditionsWithSomeExtraFTs = (mapConditionObjectsFT.size() == 0 && fts.size() != 0);

		if (isEmptyConditionsWithSomeExtraFTs) {
			matchFT = failed_match;
		}

		for (auto itr_FT = mapConditionObjectsFT.begin(); itr_FT != mapConditionObjectsFT.end(); itr_FT++)
		{
			const auto&[object_id, conditions_for_one_object_in_box] = *itr_FT;

			matchFT = match_oneFTCondition_with_FTs(owner, conditions_for_one_object_in_box, object_id, fts);

			// if condition no have match with any fts items then failed match
			if (matchFT == failed_match)
			{
				break;
			}
		}

		return matchFT;
	}

	match match_oneFTCondition_with_FTs(name owner, const vector<condition> & one_object_conditions, uint64_t object_id, const vector<asset_ex>& fts)
	{
		match matchFT = failed_match;
		const auto author_condition = find_condition(one_object_conditions, "author");
		const auto quantity_condition = find_condition(one_object_conditions, "quantity");
		const auto assettype_condition = find_condition(one_object_conditions, "assettype");

		check(!(author_condition == std::nullopt), "author does not exist in conditions of " + to_string(object_id));
		check(!(quantity_condition == std::nullopt), "quantity does not exist in conditions of object id:" + to_string(object_id));
		check(!(assettype_condition == std::nullopt), "assettype does not exist in conditions of " + to_string(object_id));
		check(!(isinteger(assettype_condition->value) == false), "assettype must be integer value. You entered: " + assettype_condition->value);

		const auto & int_condition_assettype = atoi(assettype_condition->value.c_str());
		const auto & author_condition_name = name(author_condition->value);
		const auto & quantity_condition_asset = asset_from_string(quantity_condition->value);
		const auto & assettype_condition_int = atoi(assettype_condition->value.c_str());

		if (hasLogging) { print("\n assettype_condition_int: ", assettype_condition_int); }

		const auto& buyer_inventory_index = sinventory_.template get_index<"owner"_n>();

		for (auto itr_ft = fts.begin(); itr_ft != fts.end(); itr_ft++)
		{
			if (itr_ft->assettype == assettype_condition_int && itr_ft->author == author_condition_name && itr_ft->quantity.symbol.code() == quantity_condition_asset.symbol.code())
			{
				// loop over buyer inventory and check conditions for each item
				for (auto itr_buyer_inventory = buyer_inventory_index.find(owner.value); itr_buyer_inventory != buyer_inventory_index.end(); itr_buyer_inventory++)
				{
					if (owner.value != itr_buyer_inventory->owner.value) {
						break;
					}

					if (itr_buyer_inventory->assettype == assettype_condition_int && itr_buyer_inventory->author == author_condition_name && itr_buyer_inventory->quantity.symbol.code() == quantity_condition_asset.symbol.code())
					{
						check(!(itr_buyer_inventory->quantity.amount < itr_ft->quantity.amount), "Not enough amount for fungible token of author: " + itr_ft->author.to_string() + " asset: " + itr_ft->quantity.to_string() + " . Buyer " + owner.to_string() + " balance: " + itr_buyer_inventory->quantity.to_string());

						check(!(itr_buyer_inventory->quantity.symbol.precision() != itr_ft->quantity.symbol.precision()),
							"Wrong precision for ft parameter. Inventory id: " + to_string(itr_buyer_inventory->id) + ". Symbol: " + itr_buyer_inventory->quantity.symbol.code().to_string() +
							" has precision " + to_string(itr_buyer_inventory->quantity.symbol.precision()) + ". You entered precision: " +
							to_string(itr_ft->quantity.symbol.precision()));

						// move this check to syntaxis checker
						check(!(itr_buyer_inventory->quantity.symbol.precision() != quantity_condition_asset.symbol.precision()),
							"Wrong precision for condition. Inventory id: " + to_string(itr_buyer_inventory->id) + ". Symbol: " + itr_buyer_inventory->quantity.symbol.code().to_string() +
							" has precision " + to_string(itr_buyer_inventory->quantity.symbol.precision()) + ". You entered precision: " +
							to_string(quantity_condition_asset.symbol.precision()));

						// propose less then in condition. no match 
						if (itr_buyer_inventory->quantity.amount < quantity_condition_asset.amount)
						{
							return failed_match;
						}

						if (hasLogging) { print("\n itr_buyer_inventory->id: ", itr_buyer_inventory->id); }
						matchFT = has_match;
						break;
					}
				}
			}
		}

		if (hasLogging)
		{
			print("\n FT does not match. assettype = " + to_string(assettype_condition_int) +
				" author = " + author_condition_name.to_string() + " asset = " + quantity_condition_asset.to_string());
		}

		return matchFT;
	}

	bool ismatchNFT(const name& owner, const condition &onecondition, const name& inventory_author, const unsigned& inventory_assettype, const name& inventory_category, const name& inventory_owner, const uint64_t& inventory_aid, const string& inventory_mdata, const string& inventory_idata)
	{
		auto match = false;
		const auto&[condition_key, condition_operation, condition_value] = onecondition;

		if (condition_key == "author")
		{
			if (condition_operation == "=")
			{
				if (name(condition_value) == inventory_author)
				{
					match = true;
					if (hasLogging) { print("\n author match"); print_condition(onecondition); }
				}
				else
				{
					if (hasLogging) { print("\n author no match inventory_author: " + inventory_author.to_string() + " author: " + condition_value);  print_condition(onecondition); }
				}
			}
			else
			{
				check(false, "Condition for author can be only '= operator'");
			}
		}
		else if (condition_key == "assettype")
		{
			if (condition_operation == "=")
			{
				if (atoi(condition_value.c_str()) == inventory_assettype)
				{
					match = true;
					if (hasLogging) { print("\n assettype match"); print_condition(onecondition); }
				}
				else
				{
					if (hasLogging) { print("\n assettype no match inventory_assettype: " + to_string(inventory_assettype) + " assettype: " + condition_value);  print_condition(onecondition); }
				}
			}
			else
			{
				check(false, "Condition for assettype can be only '= operator'");
			}
		}
		else if (condition_key == "category")
		{
			if (condition_operation == "=")
			{
				if (inventory_category == name(condition_value))
				{
					match = true;
					if (hasLogging) { print("\n category match"); print_condition(onecondition); }
				}
				else
				{
					if (hasLogging) { print("\n category no match"); print_condition(onecondition); }
				}
			}
			else
			{
				check(false, "Condition for category can be only '= operator'");
			}
		}
		else if (condition_key == "owner")
		{
			if (condition_operation == "=")
			{
				if (inventory_owner == name(condition_value))
				{
					match = true;
					if (hasLogging) { print("\n owner match"); print_condition(onecondition); }
				}
				else
				{
					if (hasLogging) { print("\n owner no match"); print_condition(onecondition); }
				}
			}
			else
			{
				check(false, "Condition for owner can be only '= operator'");
			}
		}
		else if (condition_key == "id")
		{
			if (condition_operation == "=")
			{
				if (to_string(inventory_aid) == condition_value)
				{
					match = true;
					if (hasLogging) { print("\n id match"); print_condition(onecondition); }
				}
				else
				{
					if (hasLogging) { print("\n id no match itraid: ", inventory_aid, " aid: ", condition_value); print_condition(onecondition); }
				}
			}
			else
			{
				check(false, "Condition for id can be only '= operator'");
			}
		}
		else
		{
			if (condition_key.find("mdata") != string::npos)
			{
				if (hasLogging) { print("\n mdata: ", inventory_mdata); }

				match = isdatamatch(onecondition, inventory_mdata);
			}
			else if (condition_key.find("idata") != string::npos)
			{
				if (hasLogging) { print("\n idata: ", inventory_idata); }

				match = isdatamatch(onecondition, inventory_idata);
			}
			else
			{
				if (hasLogging) { print("\n mdata, idata no match"); print_condition(onecondition); }
			}
		}

		return match;
	}

	bool isdatamatch(const condition& one_condition, const string& imdata)
	{
		auto match = false;
		const auto &[key_condition, operation_condition, value_condition] = one_condition;

		if (key_condition.find(".") == string::npos)
		{
			if (operation_condition == "=")
			{
				if (imdata == value_condition)
				{
					match = true;
				}
			}
			else if (operation_condition == "!=")
			{
				if (imdata != value_condition)
				{
					match = true;
				}
			}
		}
		else
		{
			string value_inventory;

			if (hasLogging) { print("\n imdata: ", imdata); }

			if (get_value(key_condition, imdata, value_inventory))
			{
				if (isinteger(value_inventory))
				{
					const auto digit_value_inventory = atoi(value_inventory.c_str());
					const auto digit_value_condition = atoi(value_condition.c_str());

					if (operation_condition == "=")
					{
						if (digit_value_inventory == digit_value_condition)
						{
							match = true;
						}
					}
					else if (operation_condition == "!=")
					{
						if (digit_value_inventory != digit_value_condition)
						{
							match = true;
						}
					}
					else if (operation_condition == ">")
					{
						if (digit_value_inventory > digit_value_condition)
						{
							match = true;
						}
					}
					else if (operation_condition == "<")
					{
						if (digit_value_inventory < digit_value_condition)
						{
							match = true;
						}
					}
					else if (operation_condition == "<=")
					{
						if (digit_value_inventory <= digit_value_condition)
						{
							match = true;
						}
					}
					else if (operation_condition == ">=")
					{
						if (digit_value_inventory >= digit_value_condition)
						{
							match = true;
						}
					}
				}
				else
				{
					if (operation_condition == "=")
					{
						if (value_inventory == value_condition)
						{
							match = true;
						}
					}
					else if (operation_condition == "!=")
					{
						if (value_inventory != value_condition)
						{
							match = true;
						}
					}
					else
					{
						check(false, "for string awailable only = and != operations " + key_condition + " " + operation_condition + " " + value_condition);
					}
				}
			}
		}

		return match;
	}

	bool get_value(const string& filter, const string& imdata, string& result)
	{
		if (hasLogging) {
			print("\n get_value");
			print("\n filter: ", filter);
			print("\n imdata: ", imdata);
		}

		vector<string> container;
		split(filter, container);

		if (hasLogging) { print("\n container.size(): ", container.size()); }

		auto it = container.begin();

		if (it == container.end())
		{
			if (hasLogging) { print("\n it == container.end()"); }
			return false;
		}

		if (!(*it == "mdata" || *it == "idata"))
		{
			if (hasLogging) { print("\n mdata || *it == idata"); }
			return false;
		}

		if (container.size() == 1)
		{
			if (hasLogging) {
				print("\n container.size() == 1");
				print("\n imdata: ", imdata);
			}

			result = imdata;
			return true;
		}

		// skip first element (mdata or idata)
		++it;

		if (it == container.end())
		{
			if (hasLogging) { print("\n if (it == container.end())"); }
			return false;
		}

		auto imdatajson = json::parse(imdata, nullptr, false);

		// unable to parse json
		if (imdatajson.dump() == "<discarded>")
		{
			if (hasLogging) { print("\n imdatajson.dump() == <discarded>"); }
			return false;
		}

		auto json = imdatajson.at(*it);

		if (hasLogging) { print("\n json.dump(): ", json.dump()); }

		++it;
		for (; it != container.end(); ++it)
		{
			json = json.at(*it);
		}

		if (json.is_null()) {
			return false;
		}

		result = json.dump();
		if (hasLogging) {
			print("\n json.dump(): ", json.dump());
		}

		return true;
	}

	match match_NFTsConditions_with_NFTs(name owner, const vector<nft_id_t>& nfts)
	{
		match matchNFT = no_need_match;

		const auto isEmptyConditionsWithSomeExtraNFTs = (mapConditionObjectsNFT.size() == 0 && nfts.size() != 0);

		if (isEmptyConditionsWithSomeExtraNFTs) {
			matchNFT = failed_match;
		}

		for (auto itr_NFT = mapConditionObjectsNFT.begin(); itr_NFT != mapConditionObjectsNFT.end(); itr_NFT++)
		{
			const auto& conditions_for_one_object_in_box = itr_NFT->second;
			const auto buyer_matched_inventoryid = match_oneNFTCondition_with_NFTs(owner, conditions_for_one_object_in_box, nfts);

			if (hasLogging) {
				print("\n buyer_matched_inventoryid: ", *buyer_matched_inventoryid);
				print("\n buyer_matched_inventoryid == std::nullopt: ", buyer_matched_inventoryid == std::nullopt);
				if (buyer_matched_inventoryid == std::nullopt) {
					print("\n !!!! no have match for objectid: " + to_string(itr_NFT->first));
				}
			}

			if (buyer_matched_inventoryid == std::nullopt) {
				return failed_match;
			}

			matchNFT = has_match;
			// complete match
			matched_nfts.push_back(*buyer_matched_inventoryid);
		}

		return matchNFT;
	}

	optional<uint64_t> match_oneNFTCondition_with_NFTs(name owner, const vector<condition> & aconditions, const vector<nft_id_t>& nfts)
	{
		optional<uint64_t> result = std::nullopt;
		auto match = false;
		// loop over buyer inventory and check conditions for each item
		for (auto itr_nft = nfts.begin(); itr_nft != nfts.end(); itr_nft++)
		{
			const auto aid_index = sinventory_.template get_index<"aid"_n>();
			const auto itr_aid = aid_index.find(*itr_nft);

			check(!(itr_aid == aid_index.end()), "Asset id = " + to_string(*itr_nft) + " does not exist in inventory");
			check(!(itr_aid->aid != *itr_nft), "Asset id : " + to_string(*itr_nft) + " does not exist in inventory");

			if (hasLogging) { print("\n next inventory item"); }
			int counter = 0;
			// loop over all conditions at ONE object in ONE box 
			for (auto itr_condition = aconditions.begin(); itr_condition != aconditions.end(); itr_condition++)
			{
				const auto author_condition = find_condition(aconditions, "author");
				const auto assettype_condition = find_condition(aconditions, "assettype");
				const auto id_condition = find_condition(aconditions, "id");

				// if have 'id' condition no need author and 
				if (id_condition == std::nullopt) {
					// break if no author 
					check(!(author_condition == std::nullopt), "author does not exist in condition");
				}

				// break if no assettype
				check(!(assettype_condition == std::nullopt), "assettype does not exist in condition");

				const auto &[category, idata, mdata] = get_sa_data(itr_aid->aid);

				if (hasLogging) {
					const auto& author_condition_value = author_condition->value;
					const unsigned& assettype_condition_value = atoi(assettype_condition->value.c_str());

					print("\n author_from_condition: ", author_condition_value);
					print("\n assettype_from_condition: ", assettype_condition_value);
					print("\n itr_aid->author: ", itr_aid->author.to_string());
					print("\n buyer category: ", category.to_string());
					print("\n itr_aid->aid: ", itr_aid->aid);
					print("\n itr_aid->id: ", itr_aid->id);
					print("\n buyer mdata: ", mdata);
					print("\n buyer idata: ", idata);
				}

				if (ismatchNFT(owner, *itr_condition, itr_aid->author, itr_aid->assettype, category, itr_aid->owner, itr_aid->aid, mdata, idata))
				{
					if (find(matched_nfts.begin(), matched_nfts.end(), itr_aid->id) == matched_nfts.end())
					{
						if (hasLogging) { print("\n match == true"); }
						match = true;
					}
					else // skip
					{
						if (hasLogging) { print("\n double ismatchNFT: false "); }

						match = false;
						break;
						// item was matched before
						// for example 1 kolobok -> (prize , prize)
						// need to find one more prize
					}
				}
				else
				{
					if (hasLogging) { print("\n ismatchNFT: false "); }
					match = false;
					break;
				}
			}

			// if all conditions fit then add to final vector
			// and move to next element in box
			if (match)
			{
				if (hasLogging) { print("\n final match = true itr_aid->id: " + to_string(itr_aid->id)); }
				result = optional<uint64_t>(itr_aid->id);
				break;
			}
		}

		return result;
	}

	tuple<name, string, string> get_sa_data(uint64_t id)
	{
		const auto& idxk = assets_.require_find(id, string("Asset id: " + to_string(id) + " was not found").c_str());
		return { idxk->category, idxk->idata, idxk->mdata };
	}

	optional<name> get_nft_author(uint64_t proposal_id)
	{
		optional<name> result = std::nullopt;

		const auto& itr_proposal = stproposals_.require_find(proposal_id, string("Proposal id " + to_string(proposal_id) + " does not exist").c_str());

		const auto& inventory_index = sinventory_.template get_index<"owner"_n>();

		for (auto itr_inventory = inventory_index.find(itr_proposal->owner.value); itr_inventory != inventory_index.end(); itr_inventory++)
		{
			if (itr_proposal->owner.value != itr_inventory->owner.value) {
				break;
			}

			for (auto itr_inprop = itr_inventory->inproposal.begin(); itr_inprop != itr_inventory->inproposal.end(); itr_inprop++)
			{
				if (*itr_inprop == proposal_id && itr_inventory->assettype == SA_NFT)
				{
					// for nft just exchange owner
					result = optional<name>(itr_inventory->author);
					break;
				}
			}
		}

		return result;
	}

	void remove_all_offers_for_proposal(const uint64_t& proposal_id)
	{
		const auto& topropid_index = stproposals_.template get_index<"topropid"_n>();

		// loop over offers and remove them
		for (auto itrprop = topropid_index.find(proposal_id); itrprop != topropid_index.end(); itrprop++)
		{
			if (proposal_id != itrprop->topropid) {
				break;
			}

			remove_proposal_with_conditions(itrprop->id);
		}
	}

	void modify_inproposal(name owner, uint64_t proposal_id)
	{
		if (hasLogging) { print("\n owner ", owner, " proposal_id ", proposal_id); }

		const auto& inventory_byowner = sinventory_.template get_index<"owner"_n>();

		// loop over owner inventory and remove for each item proposal in inproposal
		for (auto itr_inventory_byowner = inventory_byowner.find(owner.value); itr_inventory_byowner != inventory_byowner.end(); itr_inventory_byowner++)
		{
			if (owner.value != itr_inventory_byowner->owner.value) {
				break;
			}

			vector<proposal_id_t> new_inproposal;
			// loop over all inproposal items for one item from inventory table
			auto need_modify = false;
			for (auto itr_inprop = itr_inventory_byowner->inproposal.begin(); itr_inprop != itr_inventory_byowner->inproposal.end(); itr_inprop++)
			{
				if (proposal_id != *itr_inprop)
				{
					new_inproposal.push_back(*itr_inprop);
					if (hasLogging) { print("\n new_inproposal.push_back(*itr_inprop) ", *itr_inprop); }

				}
				else
				{
					if (hasLogging) { print("\n need_modify = true; removed proposal_id = ", proposal_id); }

					need_modify = true;
				}
			}

			if (need_modify) {
				const auto& itr_inventory = sinventory_.require_find(itr_inventory_byowner->id, ((string)("Inventory id = " + to_string(itr_inventory_byowner->id) + " does not exist ")).c_str());
				sinventory_.modify(itr_inventory, get_self(), [&](auto& s) {
					s.inproposal = new_inproposal;
				});
			}
		}
	}

	void remove_proposal_with_conditions(const uint64_t& proposal_id)
	{
		auto itr_proposal_item = stproposals_.find(proposal_id);

		if (itr_proposal_item != stproposals_.end())
		{
			const auto& inventory_byowner = sinventory_.template get_index<"owner"_n>();

			// loop over all conditions with proposal id from itr_inventory->inproposal
			auto proposalid_index = sconditions_.template get_index<"proposalid"_n>();
			for (auto itrprop = proposalid_index.find(proposal_id); itrprop != proposalid_index.end(); )
			{
				if (proposal_id != itrprop->proposalid) {
					break;
				}

				itrprop = proposalid_index.erase(itrprop);
			}

			stproposals_.erase(itr_proposal_item);
		}
	}

	void clean_proposal(const uint64_t& proposal_id)
	{
		if (hasLogging) { print("\n clean_proposal_for_ft "); }

		// modify inventory items inproposal field
		// remove proposal from inproposal field
		auto itr_proposal_item = stproposals_.find(proposal_id);
		if (itr_proposal_item != stproposals_.end()) {
			if (hasLogging) { print("\n clean_proposal_for_ft2 ", proposal_id); }

			modify_inproposal(itr_proposal_item->owner, proposal_id);
		}

		// remove proposal and it conditions
		remove_proposal_with_conditions(proposal_id);
		if (hasLogging) { print("\n clean_proposal_for_ft3 "); }

		const auto& topropid_index = stproposals_.template get_index<"topropid"_n>();

		vector<uint64_t> proposal_remove;
		// loop over all offers that refer to proposal and remove them
		for (auto itrprop = topropid_index.find(proposal_id); itrprop != topropid_index.end(); itrprop++)
		{
			if (proposal_id != itrprop->topropid) {
				break;
			}

			auto itr_proposal_item = stproposals_.find(itrprop->id);
			if (hasLogging) { print("\n itrprop->id ", itrprop->id); }

			if (itr_proposal_item != stproposals_.end())
			{
				if (hasLogging) { print("\n itr_proposal_item->id ", itr_proposal_item->id); }

				modify_inproposal(itr_proposal_item->owner, itr_proposal_item->id);
			}

			proposal_remove.push_back(itrprop->id);
		}

		//	separate loop cause of Error 3160005: The table operation is not allowed
		for (auto itr_prop = proposal_remove.begin(); itr_prop != proposal_remove.end(); itr_prop++)
		{
			remove_proposal_with_conditions(*itr_prop);
		}
	}

	void remove_all_proposals_for_inventory_item(const uint64_t& inventory_id)
	{
		const auto& itr_inventory = sinventory_.find(inventory_id);

		if (itr_inventory != sinventory_.end())
		{
			// loop over all inproposal items for one item from inventory table
			for (auto itr_inprop = itr_inventory->inproposal.begin(); itr_inprop != itr_inventory->inproposal.end(); itr_inprop++)
			{
				// remove proposal and it conditions
				clean_proposal(*itr_inprop);
			}
			sinventory_.modify(itr_inventory, get_self(), [&](auto& s) {
				s.inproposal = vector<proposal_id_t>();
			});
		}
	}

	void print_condition(const condition &onecondition)
	{
		print("\n onecondition: <", onecondition.key, " , ", onecondition.operation, " , ", onecondition.value);
	}
};
