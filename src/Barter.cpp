#include <Barter.hpp>

bool Barter::isdatamatch(const tuple<key_t, operation_t, value_t>& condition, const string& imdata)
{
	auto match = false;
	const auto &[key_condition, operation_condition, value_condition] = condition;

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

bool Barter::get_value(const string& filter, const string& imdata, string& result)
{
	if (hasLogging) {
		print("\n get_value");
		print("\n filter: ", filter);
		print("\n imdata: ", imdata);
	}

	vector<string> container;
	split(filter, container);

	if (hasLogging) { print("\n container.size(): ", container.size());	}

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

	if (hasLogging) { print("\n json.dump(): ", json.dump());}

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

ACTION Barter::getversion() {

	string symbol;
	#ifdef EOS_CHAIN
	symbol = "EOS";
	#endif

	#ifdef WAX_CHAIN
	symbol = "WAX";
	#endif

	string versio_info = "Version number 1.0.9, Symbol: " + symbol + string(". Build date: 2020-03-30 15:40 ") + (hasLogging == true ? "with logging" : "without logging")
		+ string(". simpleasset: ") + SIMPLEASSETS_CONTRACT.to_string() + " eosio.token: " + EOSIO_TOKEN.to_string();
#ifdef DEBUG
	versio_info += "Debug " + versio_info;
#endif // DEBUG

	check(false, versio_info);
}

ACTION Barter::rejectoffer(name owner, uint64_t offer_id)
{
	require_auth(owner);

	auto itr = stproposals_.require_find(offer_id, string("Wrong offer id: " + to_string(offer_id)).c_str());

	auto itr_topropid = stproposals_.require_find(itr->topropid, 
		string("Proposal id: " + to_string(itr->topropid) + " from offer id: " + to_string(offer_id) + " does not exist").c_str());

	check(!(itr_topropid->owner != owner), "You must be owner of proposal id: " + to_string(itr_topropid->id) + " to cancel offer id: " + to_string(offer_id) + ". Owner is: " + itr_topropid->owner.to_string() + "you entered: " + owner.to_string());

	remove_proposal_with_conditions(offer_id);
}

void Barter::create_conditions(const name& owner, const uint64_t& proposal_id, const vector<tuple<box_id_t, object_id_t, key_t, operation_t, value_t>>& conditions)
{
	map < tuple < box_id_t, object_id_t >, vector<tuple<key_t, operation_t, value_t>>> mapconditions;

	for (auto i = 0; i < conditions.size(); i++)
	{
		const auto& condition = conditions[i];

		const auto&[boxid, objectid, key, operation, value] = condition;

		mapconditions[make_tuple(boxid, objectid)].push_back(make_tuple(key, operation, value));
	}

	// logic check for condition
	checker.syntaxis_checker(mapconditions);

	for (auto itr_map = mapconditions.begin(); itr_map != mapconditions.end(); itr_map++)
	{
		const auto& box_id    = get<0>(itr_map->first);
		const auto& object_id = get<1>(itr_map->first);

		const auto &aconditions = itr_map->second;

		sconditions_.emplace(get_self(), [&](auto& g) {
			g.id          = sconditions_.available_primary_key();
			g.owner       = owner;
			g.proposalid  = proposal_id;
			g.boxid       = box_id;
			g.objectid    = object_id;
			g.aconditions = aconditions;
		});
	}
}

ACTION Barter::delblacklist(name blacklisted_author)
{
	require_auth(get_self());

	sblacklist_.erase(sblacklist_.require_find(blacklisted_author.value, 
		string("Author: " + blacklisted_author.to_string() + " does not in blacklist").c_str()));
}

ACTION Barter::addblacklist(name blacklisted_author, string memo)
{
	require_auth(get_self());
	check(!(memo.size() > MEMO_MAX_SIZE), "memo has more than " + to_string(MEMO_MAX_SIZE) + " bytes");

	sblacklist_.emplace(get_self(), [&](auto& g) {
		g.author = blacklisted_author;
		g.memo   = memo;
	});
}

ACTION Barter::createwish(name owner, vector<tuple<key_t, operation_t, value_t>>& wish_conditions)
{
	require_auth(owner);
	check(!(wish_conditions.size() == 0), warning_must_be_condition);

	swish_.emplace(get_self(), [&](auto& g) {
		g.id         = swish_.available_primary_key();
		g.owner      = owner;
		g.conditions = wish_conditions;
	});
}

ACTION Barter::cancelwish(name owner, uint64_t wish_id)
{
	require_auth(owner);
	const auto itr = swish_.require_find(wish_id, string("Wish_id: " + to_string(wish_id) + " does not exist").c_str());
	check(!(itr->owner != owner), "Wrong owner for wish id: " + to_string(wish_id) + ". You entered owner: " + owner.to_string() + " but wish belong to: " + itr->owner.to_string());

	swish_.erase(itr);
}

ACTION Barter::createprop(name owner, vector<nft_id_t>& nfts, vector<tuple<name, asset, assettype_t>>& fts, vector<tuple<box_id_t, object_id_t, key_t, operation_t, value_t>>& conditions, exchange_fees fees, uint64_t topropid, name account_to, uint64_t datestart, uint64_t dateexpire, bool auto_accept, string memo)
{
	require_auth(owner);
	check(!(nfts.size() == 0 && fts.size() == 0), errorEmptyFT_NFT);
	check(!(memo.size() > MEMO_MAX_SIZE), "Parameter memo has more than "+ to_string(MEMO_MAX_SIZE) +" bytes. You entered size = " + to_string(memo.size()));
	check(!(conditions.size() == 0), "Parameter conditions cannot be empty");

	string dublicated_nft;
	check(!(checker.hasDublicatedNFT(nfts, dublicated_nft)), "nfts has dublicated id: " + dublicated_nft);

	string dublicated_ft;
	check(!(checker.hasDublicatedFT(fts, dublicated_ft)), "fts has dublicated item: " + dublicated_ft);

	// offer functionality
	if (topropid != 0)
	{
		check(!(stproposals_.require_find(topropid, string("Proposalid: " + to_string(topropid) + " does not exist").c_str())->topropid != 0), "Cannot create offer to offer");
	}

	const auto new_proposal_id = stproposals_.available_primary_key();

	create_conditions(owner, new_proposal_id, conditions);
	
	if (hasLogging) { print("\n new_proposal_id :" + to_string(new_proposal_id)); }
	
	name last_author = name("");
	uint64_t last_aid = 0;
	
	for (auto i = 0; i < nfts.size(); i++)
	{
		const auto& nft_id    = nfts[i];
		const auto aid_index  = sinventory_.template get_index<"aid"_n>();
		const auto itr_aid    = aid_index.find(nft_id);

		check(!(itr_aid        == aid_index.end()), "Wrong assetid: " + to_string(nft_id));
		check(!(itr_aid->aid   != nft_id), "Wrong assetid: " + to_string(nft_id));
		check(!(itr_aid->owner != owner), "Wrong owner for assetid: " + to_string(nft_id) + ". You entered owner: " + owner.to_string() + " but asset belong to: " + itr_aid->owner.to_string());

		fees.checkfee(fee_processor.getAuthorFee(itr_aid->author));

		auto itr_seller_inventory = sinventory_.require_find(itr_aid->id, string("Inventory item does not exist " + to_string(itr_aid->id)).c_str());

		check( !(last_author != name("") && last_author != itr_seller_inventory->author), "Not allowed to create proposal with nfts from different authors. You have two different authors: " + last_author.to_string() + " for nft id: " + to_string(last_aid) + " and " + itr_seller_inventory->author.to_string() + " for nft id:" + to_string(itr_aid->id));

		last_author = itr_seller_inventory->author;
		last_aid     = itr_aid->aid;

		if (itr_seller_inventory != sinventory_.end())
		{
			sinventory_.modify(itr_seller_inventory, owner, [&](auto& s) {
				s.inproposal.push_back(new_proposal_id);
			});
		}
	}

	for (auto j = 0; j < fts.size(); j++)
	{
		const auto ft_id           = fts[j];
		const auto&[author_from_proposal, asset_from_proposal, assettype_from_proposal] = ft_id;
		const auto& owner_index    = sinventory_.template get_index<"owner"_n>();

		bool found = false;
		// loop over seller inventory and check conditions for each item
		for (auto itr_byowner_inventory = owner_index.find(owner.value); itr_byowner_inventory != owner_index.end(); itr_byowner_inventory++)
		{
			if (owner.value != itr_byowner_inventory->owner.value) {
				break;
			}

			if (itr_byowner_inventory->assettype == assettype_from_proposal && itr_byowner_inventory->author == author_from_proposal && itr_byowner_inventory->quantity.symbol.code() == asset_from_proposal.symbol.code())
			{
				check(!(asset_from_proposal.symbol.precision() != itr_byowner_inventory->quantity.symbol.precision()), "Incorrect precision. You entered quantity with precision: "
					+ to_string(asset_from_proposal.symbol.precision()) + ". At inventory precision: " + to_string(itr_byowner_inventory->quantity.symbol.precision())
					+ ". For author: " + itr_byowner_inventory->author.to_string() + " asset: "+ itr_byowner_inventory->quantity.to_string());

				if (hasLogging) {
					print("\n asset.amount: ", asset_from_proposal.amount);
					print("\n itr_byowner_inventory->quantity.amount: ", itr_byowner_inventory->quantity.amount);
				}

				check(!(asset_from_proposal.amount > itr_byowner_inventory->quantity.amount), "Not enough to create proposal with " + asset_from_proposal.to_string() + " from author: " + author_from_proposal.to_string() + " You have at balance: " + itr_byowner_inventory->quantity.to_string());

				auto itr_seller_inventory = sinventory_.find(itr_byowner_inventory->id);
				if (itr_seller_inventory != sinventory_.end())
				{
					sinventory_.modify(itr_seller_inventory, owner, [&](auto& s) {
						s.inproposal.push_back(new_proposal_id);
					});
					found = true;
				}
			}
		}

		check(found, "Asset not found: assettype: " + to_string(assettype_from_proposal) + " author: " + author_from_proposal.to_string() + " symbol: " + asset_from_proposal.symbol.code().to_string());
	}

	stproposals_.emplace(get_self(), [&](auto& g) {
		g.id         = new_proposal_id;
		g.owner      = owner;
		g.nfts       = nfts;
		g.fts        = fts;
		g.fees       = fees;
		g.topropid   = topropid;
		g.toaccount  = account_to;
		g.datestart  = datestart;
		g.dateexpire = dateexpire;
		g.memo       = memo;
 
		if (topropid != 0) {
			g.auto_accept = true;
		}
		else
		{
			g.auto_accept = auto_accept;
		}
	}); 
}

void Barter::remove_all_offers_for_proposal(const uint64_t& proposal_id)
{
	const auto& topropid_index = stproposals_.template get_index<"topropid"_n>();

	// loop over offers and remove them
	for (auto itrprop = topropid_index.find(proposal_id); itrprop != topropid_index.end(); itrprop++)
	{
		if (proposal_id != itrprop->id) {
			break;
		}

		remove_proposal_with_conditions(itrprop->id);
	}
}

void Barter::remove_proposal_with_conditions(const uint64_t& proposal_id)
{
	auto itr_proposal_item = stproposals_.find(proposal_id);

	if (itr_proposal_item != stproposals_.end())
	{
		// TABLE conditions{
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

void Barter::remove_all_proposals_for_inventory_item(const uint64_t& inventory_id)
{
	const auto& itr_inventory = sinventory_.find(inventory_id);

	if (itr_inventory != sinventory_.end())
	{
		// loop over all inproposal items for one item from inventory table
		for (auto itr_inprop = itr_inventory->inproposal.begin(); itr_inprop != itr_inventory->inproposal.end(); itr_inprop++)
		{
			// remove proposal and it conditions
			remove_proposal_with_conditions(*itr_inprop);
			remove_all_offers_for_proposal(*itr_inprop);
		}
		sinventory_.modify(itr_inventory, _self, [&](auto& s) {
			s.inproposal = vector<uint64_t>();
		});
	}
}

void Barter::print_tuple(const tuple<key_t, operation_t, value_t> &onecondition)
{
	print("\n onecondition: <", get<0>(onecondition), " , ", get<1>(onecondition), " , ", get<2>(onecondition));
}

bool Barter::ismatchNFT(const name& owner, const tuple<key_t, operation_t, value_t> &onecondition, const name& inventory_author, const unsigned& inventory_assettype, const name& inventory_category, const name& inventory_owner, const uint64_t& inventory_aid, const string& inventory_mdata, const string& inventory_idata)
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
				if (hasLogging) { print("\n author match"); print_tuple(onecondition); }
			}
			else
			{
				if (hasLogging) { print("\n author no match inventory_author: " + inventory_author.to_string() + " author: " + condition_value);  print_tuple(onecondition);}
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
				if (hasLogging) { print("\n assettype match"); print_tuple(onecondition); }
			}
			else
			{
				if (hasLogging) { print("\n assettype no match inventory_assettype: " + to_string(inventory_assettype) + " assettype: " + condition_value);  print_tuple(onecondition); }
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
				if (hasLogging) { print("\n category match"); print_tuple(onecondition);}
			}
			else
			{
				if (hasLogging) { print("\n category no match"); print_tuple(onecondition); }
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
				if (hasLogging) { print("\n owner match"); print_tuple(onecondition);}
			}
			else
			{
				if (hasLogging) { print("\n owner no match"); print_tuple(onecondition); }
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
				if (hasLogging) { print("\n id match"); print_tuple(onecondition);}
			}
			else
			{
				if (hasLogging) { print("\n id no match itraid: ", inventory_aid, " aid: " , condition_value); print_tuple(onecondition);}
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
			if (hasLogging) {print("\n idata: ", inventory_idata); }

			match = isdatamatch(onecondition, inventory_idata);
		}
		else
		{
			if (hasLogging) { print("\n mdata, idata no match"); print_tuple(onecondition);}
		}
	}

	return match;
}

void Barter::find_match_for_owner(const name& owner, const uint64_t& proposal_id, const uint64_t& box_id)
{
	auto itr_proposal_to_accept = stproposals_.require_find(proposal_id, string("Wrong proposal id: " + to_string(proposal_id)).c_str());
	check(!(itr_proposal_to_accept->owner == owner), "This proposal belong to you");
	check(!(itr_proposal_to_accept->toaccount != name("") && itr_proposal_to_accept->toaccount != owner), "This proposal is exclusively for account: " + itr_proposal_to_accept->toaccount.to_string());
	check(!(itr_proposal_to_accept->datestart > now()), "Proposal has start date. Time to wait: " + gettimeToWait(abs((int)((uint64_t)itr_proposal_to_accept->datestart - (uint64_t)now()))));
	check(!(itr_proposal_to_accept->dateexpire != 0 && itr_proposal_to_accept->dateexpire < now()), "Proposal is expired. It was expired: " + gettimeToWait(abs((int)((uint64_t)itr_proposal_to_accept->datestart - (uint64_t)now()))));
	check(!(itr_proposal_to_accept->auto_accept == false), "Proposal without auto-accept. Please create offer first and wait till owner will accept it");

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

			const auto assettype = atoi(((string)get<2>(*assettype_condition)).c_str());
			if (assettype == SA_FT || assettype == TOKEN)
			{
				for (auto it = itr_one_object_conditions->aconditions.begin(); it != itr_one_object_conditions->aconditions.end(); it++)
				{
					mapObjectsFT[itr_one_object_conditions->objectid].push_back(*it);
				}
			}
			else if (assettype == SA_NFT)
			{
				for (auto it = itr_one_object_conditions->aconditions.begin(); it != itr_one_object_conditions->aconditions.end(); it++)
				{
					mapObjectsNFT[itr_one_object_conditions->objectid].push_back(*it);
				}
			}
			else
			{
				check(false, "Condition does not have assettype");
			}
		}
	}

	check(!(is_box_exist == false), "Parameter box_id: " + to_string(box_id) + " does not exist for proposal id: " + to_string(proposal_id));

	for (auto itr_NFT = mapObjectsNFT.begin(); itr_NFT != mapObjectsNFT.end(); itr_NFT++)
	{
		const auto& buyer_matched_inventoryid = findNFTfromConditions(owner, itr_NFT->second);
		
		if (hasLogging) { print("\n buyer_matched_inventoryid: ", buyer_matched_inventoryid); }

		check(!(buyer_matched_inventoryid == 0), "no have match for objectid: " + to_string(itr_NFT->first));
		buyer_to_exchange.push_back(buyer_matched_inventoryid);
	}

	for (auto itr_FT = mapObjectsFT.begin(); itr_FT != mapObjectsFT.end(); itr_FT++)
	{
		const auto &[object_id, one_object_conditions] = *itr_FT;

		const auto author_condition    = find_condition(one_object_conditions, "author");
		const auto quantity_condition  = find_condition(one_object_conditions, "quantity");
		const auto assettype_condition = find_condition(one_object_conditions, "assettype");
	
		check(!(author_condition    == std::nullopt), "author does not exist in conditions of " + to_string(object_id));
		check(!(quantity_condition  == std::nullopt), "quantity does not exist in conditions of object id:" + to_string(object_id));
		check(!(assettype_condition == std::nullopt), "assettype does not exist in conditions of " + to_string(object_id));

		check(!(isinteger((string)get<2>(*assettype_condition)) == false), "assettype must be integer value. You entered: " + (string)get<2>(*assettype_condition));
		const auto & int_condition_assettype = atoi(((string)get<2>(*assettype_condition)).c_str());
		
		const auto & author_condition_name    = name((string)get<2>(*author_condition));
		const auto & quantity_condition_asset = asset_from_string((string)get<2>(*quantity_condition));
		const auto & assettype_condition_int  = atoi(((string)get<2>(*assettype_condition)).c_str());

		if (hasLogging) { print("\n assettype_condition_int: ", assettype_condition_int); }

		const auto& buyer_inventory_index = sinventory_.template get_index<"owner"_n>();

		bool matchFT = false;
		// loop over buyer inventory and check conditions for each item
		for (auto itr_buyer_inventory = buyer_inventory_index.find(owner.value); itr_buyer_inventory != buyer_inventory_index.end(); itr_buyer_inventory++)
		{
			if (owner.value != itr_buyer_inventory->owner.value) {
				break;
			}

			if (itr_buyer_inventory->assettype == assettype_condition_int && itr_buyer_inventory->author == author_condition_name && itr_buyer_inventory->quantity.symbol.code() == quantity_condition_asset.symbol.code())
			{
				check(!(itr_buyer_inventory->quantity.amount < quantity_condition_asset.amount), "Not enough amount for fungible token of author: " + author_condition_name.to_string() + " asset: " + quantity_condition_asset.to_string() + " . Buyer " + owner.to_string() + " balance: " + itr_buyer_inventory->quantity.to_string() );

				check(!(itr_buyer_inventory->quantity.symbol.precision() != quantity_condition_asset.symbol.precision()), 
					"Wrong precision. Asset id: " + to_string(itr_buyer_inventory->id) + ". Symbol: " + itr_buyer_inventory->quantity.symbol.code().to_string() + 
					" has precision " + to_string(itr_buyer_inventory->quantity.symbol.precision()) + ". You entered precision: " + 
						to_string(quantity_condition_asset.symbol.precision()));
					
				asset_to_subtract[itr_buyer_inventory->id] = quantity_condition_asset.amount;
				if (hasLogging) { print("\n itr_buyer_inventory->id: ", itr_buyer_inventory->id); }
				matchFT = true;
				buyer_to_exchange.push_back(itr_buyer_inventory->id);

				break;
			}
		}

		check(!(matchFT == false), "FT is not exist in your inventory. assettype = " + to_string(assettype_condition_int) + 
			" author = " + author_condition_name.to_string() + " asset = " + quantity_condition_asset.to_string());
	}
}

ACTION Barter::acceptprop(name owner, uint64_t proposal_id, uint64_t box_id)
{
	require_auth(owner);
	find_match_for_owner(owner, proposal_id, box_id);

	// do exchange after the end of matching
	exchange_aftermatch(owner, proposal_id);
}

void Barter::exchange_aftermatch(name owner, uint64_t proposal_id)
{
	if (hasLogging) {
		print("\n exchange_aftermatch ");
		print("\n buyer_to_exchange.size() = " + to_string(buyer_to_exchange.size()));
	}

	check(!(buyer_to_exchange.size() == 0), "Internal error. Empty buyer to exchange");

	const auto& itr_proposal_to_accept = stproposals_.require_find(proposal_id, string("Proposal id " + to_string(proposal_id) + " does not exist").c_str());
	
	//Not allowed to create proposal with nfts from different authors. so possible to get first one
	const auto nft_author = get_nft_author(itr_proposal_to_accept->owner, proposal_id);

	// move buyer items from proposal to seller inventory
	for (auto itr_buyer_to_exchange = buyer_to_exchange.begin(); itr_buyer_to_exchange != buyer_to_exchange.end(); itr_buyer_to_exchange++)
	{
		if (hasLogging) {
			print("\n *itr_buyer_to_exchange = ", *itr_buyer_to_exchange);
		}
		const auto& itr_buyer_inventory = sinventory_.find(*itr_buyer_to_exchange);
		if (itr_buyer_inventory != sinventory_.end())
		{
			if (itr_buyer_inventory->assettype == SA_NFT)
			{
				sinventory_.modify(itr_buyer_inventory, owner, [&](auto& s) {
					s.owner = itr_proposal_to_accept->owner;
				});
			}
			else if (itr_buyer_inventory->assettype == SA_FT || itr_buyer_inventory->assettype == TOKEN)
			{
				check(!(asset_to_subtract[itr_buyer_inventory->id] == 0), "Error during subtract ft. Inventory id: " + to_string(itr_buyer_inventory->id));
				if (hasLogging) {
					print("\n asset_to_subtract[", itr_buyer_inventory->id, "]: ", asset_to_subtract[itr_buyer_inventory->id]);
					print("\n itr_buyer_inventory->quantity: ", itr_buyer_inventory->quantity);
					print("\n subtract from buyer inventory item \n");	}
				// subtract from buyer inventory item 
				sinventory_.modify(itr_buyer_inventory, get_self(), [&](auto& g) {
					g.quantity -= asset(asset_to_subtract[itr_buyer_inventory->id], itr_buyer_inventory->quantity.symbol);
				});

				if (hasLogging) {
					print("\n itr_buyer_inventory->quantity: ", itr_buyer_inventory->quantity);
					print("\n move to seller account that was subtracted from buyer");
				}

				asset seller_payment = asset(asset_to_subtract[itr_buyer_inventory->id], itr_buyer_inventory->quantity.symbol);
				// subtruct here from asset payment to author and exchange fee. Exchange fee include affiliates fee
				if (nft_author != std::nullopt)
				{
					fee_processor.start(get_self(), *nft_author, itr_proposal_to_accept->fees, { itr_buyer_inventory->author, seller_payment, itr_buyer_inventory->assettype });

					if (hasLogging) {
						print("\n seller_payment:", seller_payment, " amount: ", seller_payment.amount, " precesion: ", seller_payment.symbol.precision());
					}

					// payment asset to seller with subtructed author and exchange fee. Exchange fee include affiliates fee
					seller_payment = fee_processor.getBuyerPart().quantity;

					if (hasLogging) {
						print("\n fee_processor.getBuyerPart().quantity:", fee_processor.getBuyerPart().quantity, " amount: ", fee_processor.getBuyerPart().quantity.amount, " precesion: ", fee_processor.getBuyerPart().quantity.symbol.precision());
					}

#ifdef AFFILIATEBANK
					// deposit exchange`s fee
					if (const auto exchange_fee = fee_processor.getExchangeFee(); exchange_fee.quantity.amount > 0)
					{
						depositFT(itr_proposal_to_accept->fees.exchange, exchange_fee.author, exchange_fee.quantity, exchange_fee.assettype);
					}
#endif

					const auto author_fee = fee_processor.getAuthorFee();

					if (hasLogging) {
						print("\n deposit author`s fee");
					}

					// deposit author`s fee
					if (author_fee.quantity.amount > 0)
					{
						depositFT(*nft_author, author_fee.author, author_fee.quantity, author_fee.assettype);
					}

					if (hasLogging) {
						print("\n deposit to author account. author_fee.quantity: ", author_fee.quantity);
						print("\n author: ", nft_author->to_string());
					}

				}

				// move to seller account that was subtracted from buyer
				depositFT(itr_proposal_to_accept->owner, itr_buyer_inventory->author, seller_payment, itr_buyer_inventory->assettype);
			}

			remove_all_proposals_for_inventory_item(itr_buyer_inventory->id);
		}
	}

	// move seller items to buyer
	const auto& seller_inventory_index = sinventory_.template get_index<"owner"_n>();

	for (auto itr_seller_inventory = seller_inventory_index.find(itr_proposal_to_accept->owner.value); itr_seller_inventory != seller_inventory_index.end(); itr_seller_inventory++)
	{
		if (itr_proposal_to_accept->owner.value != itr_seller_inventory->owner.value) {
			break;
		}

		for (auto itr_inprop = itr_seller_inventory->inproposal.begin(); itr_inprop != itr_seller_inventory->inproposal.end(); itr_inprop++)
		{
			// loop over all items for owner with current proposal id 
			if (*itr_inprop == proposal_id)
			{
				// for nft just exchange owner
				if (itr_seller_inventory->assettype == SA_NFT)
				{
					if (hasLogging) { print("\n proposal_id: ", proposal_id); }

					const auto& itr = sinventory_.find(itr_seller_inventory->id);
					if (itr != sinventory_.end()) {
						sinventory_.modify(itr, owner, [&](auto& s) {
							s.owner = owner;
						});
					}

					if (hasLogging) { print("\n remove inventory id: ", itr_seller_inventory->id); }

					remove_all_proposals_for_inventory_item(itr_seller_inventory->id);
					break;
				}
				else if (itr_seller_inventory->assettype == SA_FT || itr_seller_inventory->assettype == TOKEN)
				{
					// for ft calculate balance and subtract
					const auto& itr_buyer_invent = sinventory_.find(itr_seller_inventory->id);
					if (itr_buyer_invent != sinventory_.end())
					{
						for (auto itr_ft = itr_proposal_to_accept->fts.begin(); itr_ft != itr_proposal_to_accept->fts.end(); itr_ft++)
						{
							const auto& author_from_proposal    = get<0>(*itr_ft);
							const auto& asset_from_proposal     = get<1>(*itr_ft);
							const auto& assettype_from_proposal = get<2>(*itr_ft);

							if (author_from_proposal == itr_seller_inventory->author && asset_from_proposal.symbol.code() == itr_seller_inventory->quantity.symbol.code() && assettype_from_proposal == itr_seller_inventory->assettype)
							{
								if (hasLogging) {
									print("\n asset: ", asset_from_proposal);
									print("\n before subtruct itr_seller_inventory->quantity: ", itr_seller_inventory->quantity);
								}
								
								check(!(asset_from_proposal.amount > itr_seller_inventory->quantity.amount), "Not enough balance for owner: " + itr_seller_inventory->owner.to_string() );
								// subtract from their inventory for ft 
								sinventory_.modify(itr_buyer_invent, get_self(), [&](auto& g) {
									g.quantity -= asset_from_proposal;
								});
								if (hasLogging) {
									print("\n after subtruct itr_seller_inventory->quantity: ", itr_seller_inventory->quantity);
									print("\n deposit to buyer account");
								}

								// deposit to buyer account
								depositFT(owner, author_from_proposal, asset_from_proposal, itr_seller_inventory->assettype);
								remove_all_proposals_for_inventory_item(itr_seller_inventory->id);
								break;
							}
						}
					}
				}

				break;
			}
		}
	}
}

optional<name> Barter::get_nft_author(name owner, uint64_t proposal_id)
{
	optional<name> result = std::nullopt;
	// move seller items to buyer
	const auto& seller_inventory_index = sinventory_.template get_index<"owner"_n>();

	for (auto itr_seller_inventory = seller_inventory_index.find(owner.value); itr_seller_inventory != seller_inventory_index.end(); itr_seller_inventory++)
	{
		if (owner.value != itr_seller_inventory->owner.value) {
			break;
		}

		for (auto itr_inprop = itr_seller_inventory->inproposal.begin(); itr_inprop != itr_seller_inventory->inproposal.end(); itr_inprop++)
		{
			if (*itr_inprop == proposal_id && itr_seller_inventory->assettype == SA_NFT)
			{
				// for nft just exchange owner
				result = itr_seller_inventory->author;
				break;
			}
		}
	}

	return result;
}

uint64_t Barter::findNFTfromConditions(name owner, const vector<tuple<key_t, operation_t, value_t> > & aconditions)
{
	uint64_t result = 0;
	// LOOP over ONE object from selected by user box_id 
	const auto& buyer_inventory_index = sinventory_.template get_index<"owner"_n>();

	auto match = false;
	// loop over buyer inventory and check conditions for each item
	for (auto itr_buyer_inventory = buyer_inventory_index.find(owner.value); itr_buyer_inventory != buyer_inventory_index.end(); itr_buyer_inventory++)
	{
		if (owner.value != itr_buyer_inventory->owner.value) {
			break;
		}

		if (hasLogging) { print("\n next inventory item"); }
		int counter = 0;
		// loop over all conditions at ONE object in ONE box 
		for (auto ituple = aconditions.begin(); ituple != aconditions.end(); ituple++)
		{
			const auto author_condition    = find_condition(aconditions, "author");
			const auto assettype_condition = find_condition(aconditions, "assettype");
			const auto id_condition        = find_condition(aconditions, "id");

			// if have 'id' condition no need author and 
			if (id_condition == std::nullopt) {
				// break if no author 
				check(!(author_condition == std::nullopt), "author does not exist in condition");
			}

			// break if no assettype
			check(!(assettype_condition == std::nullopt), "assettype does not exist in condition");
			
			if (hasLogging) {
				const auto& author_condition_value = (string)get<2>(*author_condition);
				const unsigned& assettype_condition_value = atoi(((string)(get<2>(*assettype_condition))).c_str());
				print("\n author_from_condition: "   , author_condition_value);
				print("\n assettype_from_condition: ", assettype_condition_value);
				print("\n itr_buyer_inventory->author: "   , itr_buyer_inventory->author.to_string());
				print("\n itr_buyer_inventory->category: " , itr_buyer_inventory->category.to_string());
				print("\n itr_buyer_inventory->aid: "      , itr_buyer_inventory->aid);
				print("\n itr_buyer_inventory->id: "       , itr_buyer_inventory->id);
				print("\n itr_buyer_inventory->mdata: "    , itr_buyer_inventory->mdata);
				print("\n itr_buyer_inventory->idata: "    , itr_buyer_inventory->idata);
			}

			if (ismatchNFT(owner, *ituple, itr_buyer_inventory->author, itr_buyer_inventory->assettype, itr_buyer_inventory->category, itr_buyer_inventory->owner, itr_buyer_inventory->aid, itr_buyer_inventory->mdata, itr_buyer_inventory->idata))
			{
				if (find(buyer_to_exchange.begin(), buyer_to_exchange.end(), itr_buyer_inventory->id) == buyer_to_exchange.end())
				{
					if (hasLogging) { print("\n match == true"); }
					match = true;
				}
				else // skip
				{
					if (hasLogging) { print("\n double ismatchNFT: false "); }

					match = false;
					break;
					// item already in match 
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
			if (hasLogging) { print("\n final match = true itr_buyer_inventory->id: " + to_string(itr_buyer_inventory->id)); }
			result = itr_buyer_inventory->id;
			break;
		}
	}

	return result;
}

ACTION Barter::getbalance(name owner, name author, string sym)
{
	const auto& inventory_index = sinventory_.template get_index<"owner"_n>();

	for (auto itr_inventory = inventory_index.find(owner.value); itr_inventory != inventory_index.end(); itr_inventory++)
	{
		if (owner.value != itr_inventory->owner.value) {
			break;
		}

		if (itr_inventory->author == author && itr_inventory->quantity.symbol.code().to_string() == sym) {
			print("\n balance:" , itr_inventory->quantity, ":");
		}
	}
}

ACTION Barter::cancelprop(name owner, uint64_t proposal_id)
{
	require_auth(owner);

	const auto& itr = stproposals_.require_find(proposal_id, string("Proposal id: " + to_string(proposal_id) + " does not exist").c_str());

	check(!(itr->owner != owner), "Wrong owner for proposal id: " + to_string(proposal_id) + ". You entered owner: " + owner.to_string() + " but proposal belong to: " + itr->owner.to_string());

	const auto& owner_index = sinventory_.template get_index<"owner"_n>();

	for (auto itrowner = owner_index.find(owner.value); itrowner != owner_index.end(); itrowner++)
	{
			if (owner.value != itrowner->owner.value) {
				break;
			}

		    auto inproposal =  itrowner->inproposal;
			auto inprop_itr = find( inproposal.begin(), inproposal.end(), proposal_id );

			if (inprop_itr != inproposal.end())
			{
				const auto& itr = sinventory_.find(itrowner->id);
				if (itr != sinventory_.end()) {
					inproposal.erase(inprop_itr);
					sinventory_.modify(itr, owner, [&](auto& s) {
						s.inproposal = inproposal;
					});
				}
			}
	}

	stproposals_.erase(itr);

	auto proposalid_index = sconditions_.template get_index<"proposalid"_n>();

	for (auto itrprop = proposalid_index.find(proposal_id); itrprop != proposalid_index.end(); )
	{
		if (proposal_id != itrprop->proposalid) {
			break;
		}

		itrprop = proposalid_index.erase(itrprop);
	}
}

ACTION Barter::withdraw(name owner, vector<nft_id_t>& nfts, vector<tuple<name, asset, assettype_t>>& fts)
{
	require_auth(owner);
	require_recipient(owner);

	check(!(nfts.size() == 0 && fts.size() == 0), errorEmptyFT_NFT);

	for (auto i = 0; i < nfts.size(); i++)
	{
		const auto& nft_id  = nfts[i];
		auto aid_index      = sinventory_.template get_index<"aid"_n>();

		const auto& itr_aid = aid_index.require_find(nft_id, string("Asset does not exist in inventory. Wrong assetid: " + to_string(nft_id)).c_str());

		check(!(itr_aid->aid   != nft_id), "Wrong assetid: " + to_string(nft_id));
		check(!(itr_aid->owner != owner), "Wrong owner for assetid: " + to_string(nft_id) + ". You entered owner: " + owner.to_string() + " but asset belong to: " + itr_aid->owner.to_string());

		remove_all_proposals_for_inventory_item(itr_aid->id);
		aid_index.erase(itr_aid);
	}

	if (nfts.size() > 0)
	{
		action(
			permission_level{ get_self(), "active"_n },
			SIMPLEASSETS_CONTRACT,
			"transfer"_n,
			make_tuple(get_self(),
				owner,
				nfts,
				withdraw_memo)
		).send();
	}

	for (auto ift = 0; ift < fts.size(); ift++)
	{
		const auto& nft_id = fts[ift];
		auto myinventory_index = sinventory_.template get_index<"owner"_n>();

		const auto& withraw_author = get<0>(fts[ift]);
		const auto& withraw_asset  = get<1>(fts[ift]);
		const uint64_t& assettype  = get<2>(fts[ift]);

		for (auto itrmynventory = myinventory_index.find(owner.value); itrmynventory != myinventory_index.end(); itrmynventory++)
		{
			if (owner.value != itrmynventory->owner.value) {
				break;
			}

			if (itrmynventory->author == withraw_author && itrmynventory->quantity.symbol.code() == withraw_asset.symbol.code() && assettype == itrmynventory->assettype)
			{
				check(!(withraw_asset.amount > itrmynventory->quantity.amount), "Not enough to withdraw " + withraw_asset.to_string()+ " from author: " + withraw_asset.to_string() + " You have ");

				if (withraw_asset.amount != itrmynventory->quantity.amount)
				{
					auto itr_my_invent = sinventory_.find(itrmynventory->id);
					if (itr_my_invent != sinventory_.end())
					{
						sinventory_.modify(itr_my_invent, owner, [&](auto& s) {
							s.quantity.amount -= withraw_asset.amount;
						});
					}

					if (!canwithdraw(owner, itrmynventory->author, withraw_asset, assettype)) 
					{
						remove_all_proposals_for_inventory_item(itrmynventory->id);
					}
				}
				else
				{
					remove_all_proposals_for_inventory_item(itrmynventory->id);
					myinventory_index.erase(itrmynventory);
				}

				if (itrmynventory->assettype == TOKEN)
				{
					action(
						permission_level{ get_self(),"active"_n },
						withraw_author,
						"transfer"_n,
						make_tuple(get_self(), owner, withraw_asset, withdraw_memo)
					).send();
				}
				else
				{
					action(
						permission_level{ get_self(),"active"_n },
						SIMPLEASSETS_CONTRACT,
						"transferf"_n,
						make_tuple(get_self(), owner, withraw_author, withraw_asset, withdraw_memo)
					).send();
				}
			}
		}
	}
}

bool Barter::canwithdraw(name owner, name author, asset amount_to_withdraw, uint64_t assettype)
{
	auto result = true;

	const auto owner_index = stproposals_.template get_index<"owner"_n>();
	
	for (auto itrowner = owner_index.find(owner.value); itrowner != owner_index.end(); itrowner++)
	{
		if (owner.value != itrowner->owner.value) {
			break;
		}

		for (auto itr = itrowner->fts.begin(); itr != itrowner->fts.end(); itr++)
		{
			const auto& [author_from_proposal, asst_from_proposal, assettype_from_proposal] = *itr;

			if (assettype_from_proposal == assettype && author_from_proposal == author && asst_from_proposal.symbol.code() == amount_to_withdraw.symbol.code() && asst_from_proposal.amount > amount_to_withdraw.amount)
			{
				result = false;
				break;
			}
		}
	}
	return result;
}

void Barter::depositFT(name deposit_account, name author, asset quantity, uint64_t assettype)
{
	const auto& myinventory_index = sinventory_.template get_index<"owner"_n>();

	auto newasset = true;
	for (auto itrmynventory = myinventory_index.find(deposit_account.value); itrmynventory != myinventory_index.end(); itrmynventory++)
	{
		if (deposit_account.value != itrmynventory->owner.value) {
			break;
		}

		if (itrmynventory->author == author && itrmynventory->quantity.symbol.code() == quantity.symbol.code())
		{
			const auto& itr = sinventory_.find(itrmynventory->id);
			if (itr != sinventory_.end())
			{
				if (hasLogging) { print("\n Deposit of owner " + itr->owner.to_string() + " before modify itr->quantity:  ", itr->quantity); }

				sinventory_.modify(itr, get_self(), [&](auto& s) {
					s.quantity.amount += quantity.amount;
				});

				if (hasLogging) { print("\n Deposit of owner " + itr->owner.to_string() + " after modify itr->quantity: ", itr->quantity); }

				newasset = false;
				break;
			}
		}
	}

	if (newasset)
	{
		sinventory_.emplace(get_self(), [&](auto& g) {
			g.id        = sinventory_.available_primary_key();
			g.owner     = deposit_account;
			g.assettype = assettype;
			g.author    = author;
			g.quantity  = quantity;
		});
	}
}

void Barter::receiveASSETF(name from, name to, name author, asset quantity, string memo)
{
	if (from == get_self()) {
		return;
	}
	require_auth(from);

	check(!(sblacklist_.find(author.value) != sblacklist_.end()), "Author: " + author.to_string() + " was blacklisted");

	depositFT(from, author, quantity, SA_FT);
}

void Barter::receiveASSET(name from, name to, vector<uint64_t>& assetids, string memo) {

	if (from == get_self()) {
		return;
	}
	
	require_auth(from);
	require_recipient(from);


	for (auto i = 0; i < assetids.size(); i++) 
	{
		sassets assets(SIMPLEASSETS_CONTRACT, _self.value);

		const auto& idxk = assets.require_find(assetids[i], "Asset was not found or not yours");

		check(!(sblacklist_.find(idxk->author.value) != sblacklist_.end()), "Author: " + idxk->author.to_string() + " was blacklisted");

		sinventory_.emplace(get_self(), [&](auto& g) {
			g.id        = sinventory_.available_primary_key();
			g.owner     = from;
			g.aid       = assetids[i];
			g.assettype = SA_NFT; // NFT
			g.author    = idxk->author;
			g.category  = idxk->category;
			g.idata     = idxk->idata; // immutable data
			g.mdata     = idxk->mdata; // mutable data
		});
	}
}

void Barter::receiveToken(name from, name to, asset quantity, string memo)
{
	if (to != get_self()) {
		return;
	}

	require_auth(from);

	depositFT(from, name(Barter::code_test), quantity, TOKEN);
}

#ifdef DEBUG

ACTION Barter::tstfee(name nft_author, exchange_fees fee, asset_ex payment_ft_from_proposal)
{
	fee.checkfee(fee_processor.getAuthorFee(nft_author));
	fee_processor.start(get_self(), nft_author, fee, payment_ft_from_proposal);

	const auto buyer_payment = fee_processor.getBuyerPart();
#ifdef AFFILIATEBANK
	if (const auto exchange_fee = fee_processor.getExchangeFee(); exchange_fee.quantity.amount > 0)
	{
		depositFT(fee.exchange, exchange_fee.author, exchange_fee.quantity, exchange_fee.assettype);
	}
#endif

	if (const auto author_fee = fee_processor.getAuthorFee(); author_fee.quantity.amount > 0)
	{
		depositFT(nft_author, author_fee.author, author_fee.quantity, author_fee.assettype);
	}
}

ACTION Barter::tstcondition(const vector<tuple<box_id_t, object_id_t, key_t, operation_t, value_t>>& conditions)
{
	map < tuple < uint64_t, uint64_t>, vector<tuple<key_t, operation_t, value_t>>> mapconditions;

	for (auto i = 0; i < conditions.size(); i++)
	{
		const auto& condition = conditions[i];

		const auto&[boxid, objectid, key, operation, value] = condition;

		mapconditions[make_tuple(boxid, objectid)].push_back(make_tuple(key, operation, value));
	}

	checker.syntaxis_checker(mapconditions);
}

ACTION Barter::changetype(name owner, uint64_t inventory_id, uint64_t assettype)
{
	const auto& itr_inventory = sinventory_.find(inventory_id);
	check(!(itr_inventory == sinventory_.end()), "Inventory id " + to_string(inventory_id) + " does not exist");

	sinventory_.modify(itr_inventory, owner, [&](auto& s) {
		s.assettype = assettype;
	});
}

// erase proposals table
ACTION Barter::eraseallprop()
{
	for (auto itrprop = stproposals_.begin(); itrprop != stproposals_.end(); )
	{
		itrprop = stproposals_.erase(itrprop);
	}

	for (auto itrcond = sconditions_.begin(); itrcond != sconditions_.end(); )
	{
		itrcond = sconditions_.erase(itrcond);
	}
}

ACTION Barter::checkaccept(name owner, uint64_t proposal_id, uint64_t box_id)
{
	hasLogging = true;
	find_match_for_owner(owner, proposal_id, box_id);

	for (auto itr_FT = mapObjectsFT.begin(); itr_FT != mapObjectsFT.end(); itr_FT++)
	{
		//const auto & one_object_conditions = itr_FT->second;
		//print("\n condition ft item: ", get<0>(*itr_map), get<1>(*itr_map));
	}

	print("\n ---  Accepted items --- ");

	// move my items to proposal to accept owner
	for (auto itr_buyer_to_exchange = buyer_to_exchange.begin(); itr_buyer_to_exchange != buyer_to_exchange.end(); itr_buyer_to_exchange++)
	{
		print("\n -- id: ", *itr_buyer_to_exchange);

		const auto& itr_my_invent = sinventory_.find(*itr_buyer_to_exchange);

		if (asset_to_subtract[itr_my_invent->id] != 0)
		{
			print("\n Subtract amount " + to_string((double)((double)asset_to_subtract[itr_my_invent->id])) + " from this item ");
		}

		if (itr_my_invent != sinventory_.end())
		{
			print("\n aid: ", itr_my_invent->aid);
			print("\n category: ", itr_my_invent->category.to_string());
			print("\n author: ", itr_my_invent->author.to_string());
			print("\n idata: ", itr_my_invent->idata);
			print("\n mdata: ", itr_my_invent->mdata);
		}
	}
}

ACTION Barter::delproposal(name owner, uint64_t inventory_id)
{
	// delete all proposals and condition for inventory item
	require_auth(owner);

	// TABLE inventory{ 
	// find item in inventory table
	const auto& itr_inventory = sinventory_.find(inventory_id);
	check(!(itr_inventory == sinventory_.end()), "Inventory id " + to_string(inventory_id) + " does not exist");
	check(!(itr_inventory->owner != owner), owner.to_string() + " is not owner of inventory id: " + to_string(inventory_id) + ". Owner is " + itr_inventory->owner.to_string());

	remove_all_proposals_for_inventory_item(inventory_id);
}

ACTION Barter::delinventory(name owner, uint64_t inventory_id)
{
	// delete all proposals and condition for inventory item
	//require_auth(owner);

	// TABLE inventory{ 
	// find item in inventory table
	const auto& itr_inventory = sinventory_.find(inventory_id);
	check(!(itr_inventory == sinventory_.end()), "Inventory id " + to_string(inventory_id) + " does not exist");
	check(!(itr_inventory->owner != owner), owner.to_string() + " is not owner of inventory id: " + to_string(inventory_id) + ". Owner is " + itr_inventory->owner.to_string());

	remove_all_proposals_for_inventory_item(inventory_id);
	sinventory_.erase(itr_inventory);
}

ACTION Barter::testgetvalue(string filter, string imdata)
{
	// example of parameters:
	// filter = "mdata.cd.dd";
	// imdata = "{\"cd\":{\"dd\":10},\"health\":100,\"img\":\"https://k.cryptolions.io/0fde4208a0e20618d4968fdfbbafae25f88611d2f7178bbdc498f04a4c268dca.png\",\"kids\":0,\"name\":\"testkolob\",\"url\":\"simpleassets.io\"}";

	string result;
	const auto& res = get_value(filter, imdata, result);

	print("\n Function return: ", res, " result: ", result);
}

ACTION Barter::delcondition(name owner, uint64_t conditionid)
{
	require_auth(owner);

	const auto& itrcond = sconditions_.find(conditionid);

	check(!(itrcond == sconditions_.end()), "Wrong proposal id: " + to_string(conditionid));
	check(!(itrcond->owner != owner), "Wrong owner for condition id: " + to_string(conditionid) + ". You entered owner: " + owner.to_string() + " but condition belong to: " + itrcond->owner.to_string());

	sconditions_.erase(itrcond);
}

ACTION Barter::datamatch(uint64_t inventory_id, tuple<key_t, operation_t, value_t>& condition)
{
	const auto& itr_inventory = sinventory_.find(inventory_id);
	if (itr_inventory != sinventory_.end())
	{
		print("\n isdatamatch: " + to_string(isdatamatch(condition, itr_inventory->mdata)));
	}
}

ACTION Barter::testisint(const string& value)
{
	print("\n isinteger(value):", isinteger(value));
}

ACTION Barter::tstwithdraw(name owner, name asset_author, asset withraw_asset, uint64_t assettype)
{
	print("\n canwithdraw(owner, asset_author, withraw_asset):", canwithdraw(owner, asset_author, withraw_asset, assettype));
}

ACTION Barter::tstautfee(name author)
{
	print("\n getAuthorFee: ", fee_processor.getAuthorFee(author));
}

#endif