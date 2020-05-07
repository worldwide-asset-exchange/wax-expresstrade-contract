#include <wax_express_trade.hpp>

ACTION wax_express_trade::getversion() {

	string symbol;
#ifdef EOS_CHAIN
	symbol = "EOS";
#endif

#ifdef WAX_CHAIN
	symbol = "WAX";
#endif

	string versio_info = "Version number 1.0.9 , Symbol: " + symbol + string(". Build date: 2020-04-17 14:00 ") + (hasLogging == true ? "with logging" : "without logging")
		+ string(". simpleasset: ") + SIMPLEASSETS_CONTRACT.to_string() + " eosio.token: " + EOSIO_TOKEN.to_string();
#ifdef DEBUG
	versio_info += "Debug " + versio_info;
#endif // DEBUG

	check(false, versio_info);
}

ACTION wax_express_trade::rejectoffer(name owner, uint64_t offer_id)
{
	require_auth(owner);

	auto itr = stproposals_.require_find(offer_id, string("Wrong offer id: " + to_string(offer_id)).c_str());

	const auto &is_proposal = (itr->topropid == 0);
	check(!(is_proposal), "Cannot cancel proposal with rejectoffer action, use cancelprop action instead");

	auto itr_topropid = stproposals_.require_find(itr->topropid,
		string("Proposal id: " + to_string(itr->topropid) + " from offer id: " + to_string(offer_id) + " does not exist").c_str());

	check(!(itr_topropid->owner != owner), "You must be owner of proposal id: " + to_string(itr_topropid->id) + " to cancel offer id: " + to_string(offer_id) + ". Owner is: " + itr_topropid->owner.to_string() + "you entered: " + owner.to_string());

	acceptor_.remove_proposal_with_conditions(offer_id);
}

void wax_express_trade::create_conditions(const name& owner, const uint64_t& proposal_id, const map < tuple < box_id_t, object_id_t >, vector<condition>>& mapconditions)
{
	for (auto itr_map = mapconditions.begin(); itr_map != mapconditions.end(); itr_map++)
	{
		const auto& box_id = get<0>(itr_map->first);
		const auto& object_id = get<1>(itr_map->first);

		const auto &aconditions = itr_map->second;

		sconditions_.emplace(get_self(), [&](auto& g) {
			g.id = getid();
			g.owner = owner;
			g.proposalid = proposal_id;
			g.boxid = box_id;
			g.objectid = object_id;
			g.aconditions = aconditions;
		});
	}
}

ACTION wax_express_trade::delblacklist(name blacklisted_author)
{
	require_auth(get_self());

	sblacklist_.erase(sblacklist_.require_find(blacklisted_author.value,
		string("Author: " + blacklisted_author.to_string() + " does not in blacklist").c_str()));
}

ACTION wax_express_trade::addblacklist(name blacklisted_author, string memo)
{
	require_auth(get_self());
	check(!(memo.size() > MEMO_MAX_SIZE), "memo has more than " + to_string(MEMO_MAX_SIZE) + " bytes");

	sblacklist_.emplace(get_self(), [&](auto& g) {
		g.author = blacklisted_author;
		g.memo = memo;
	});
}

ACTION wax_express_trade::createwish(name owner, vector<condition>& wish_conditions)
{
	require_auth(owner);
	check(!(wish_conditions.size() == 0), must_be_at_least_one + warning_one_condition);

	swish_.emplace(get_self(), [&](auto& g) {
		g.id = getid();
		g.owner = owner;
		g.conditions = wish_conditions;
	});
}

ACTION wax_express_trade::cancelwish(name owner, uint64_t wish_id)
{
	require_auth(owner);
	const auto itr = swish_.require_find(wish_id, string("Wish_id: " + to_string(wish_id) + " does not exist").c_str());
	check(!(itr->owner != owner), "Wrong owner for wish id: " + to_string(wish_id) + ". You entered owner: " + owner.to_string() + " but wish belong to: " + itr->owner.to_string());

	swish_.erase(itr);
}

bool wax_express_trade::try_accept_proposal(name owner, proposal_id_t topropid, uint64_t box_id, const vector<nft_id_t>& nfts, const vector<asset_ex>& fts)
{
	bool result = false;
	// read conditions from proposal and separate in two NFTs and FT arrays
	acceptor_.separate_conditions_to_NFTs_FTs(topropid, box_id);

	// do match nfts with conditions
	auto matchNFT = acceptor_.match_NFTsConditions_with_NFTs(owner, nfts);
	auto matchFT = acceptor_.match_FTsConditions_with_FTs(owner, fts);

	if ((matchNFT == has_match || matchNFT == no_need_match) && (matchFT == has_match || matchFT == no_need_match))
	{
		const auto itr_proposal_to_accept = stproposals_.require_find(topropid, string("Wrong proposal id: " + to_string(topropid)).c_str());

		const auto& seller = owner;
		const auto& buyer = itr_proposal_to_accept->owner;

		const auto& seller_nfts = nfts;
		const auto& seller_fts = fts;

		const auto& buyer_nfts = itr_proposal_to_accept->nfts;
		const auto& buyer_fts = itr_proposal_to_accept->fts;

		// do exchange after the end of matching
		if (hasLogging) { print("\n ------------ move_items seller to buyer ------------ \n"); }
		acceptor_.move_items(owner, seller, buyer, seller_nfts, seller_fts, itr_proposal_to_accept->fees, itr_proposal_to_accept->id);

		if (hasLogging) {
			print("\n ------------ move_items buyer to seller ------------ \n");
		}
		acceptor_.move_items(owner, buyer, seller, buyer_nfts, buyer_fts, exchange_fees{}, 0);
		acceptor_.clean_proposals_offers_conditions();

		result = true;
	}
	return result;
}

ACTION wax_express_trade::createoffer(name owner, const vector<nft_id_t>& nfts, const vector<asset_ex>& fts, proposal_id_t topropid, box_id_t box_id, date_range daterange, string memo)
{
	require_auth(owner);
	const auto itrToProp = stproposals_.require_find(topropid, string("Proposal topropid = " + to_string(topropid) + " does not exist").c_str());
	check(!(itrToProp->topropid != 0), "Proposal topropid = " + to_string(itrToProp->topropid) + " is offer. Cannot create offer to offer");

	check(!(itrToProp->owner == owner), "This proposal belong to you");
	check(!(itrToProp->toaccount != name("") && itrToProp->toaccount != owner), "This proposal is exclusively for account: " + itrToProp->toaccount.to_string());
	check(!(itrToProp->daterange.datestart > now()), "Proposal has start date. Time to wait: " + gettimeToWait(abs((int)((uint64_t)itrToProp->daterange.datestart - (uint64_t)now()))));
	check(!(itrToProp->daterange.dateexpire != 0 && itrToProp->daterange.dateexpire < now()), "Proposal is expired. It was expired: " + gettimeToWait(abs((int)((uint64_t)itrToProp->daterange.datestart - (uint64_t)now()))));

	check(!(acceptor_.is_proposal_gift(topropid, itrToProp->topropid)), "Proposal id = " + to_string(itrToProp->topropid) + " is a gift. Use sendgift to send gift");

	if (itrToProp->auto_accept == false || !try_accept_proposal(owner, topropid, box_id, nfts, fts))
	{
		check(!(nfts.size() == 0 && fts.size() == 0), must_be_at_least_one + errorEmptyFT_NFT);
		create_proposal(owner, nfts, fts, vector<tuple<box_id_t, object_id_t, condition>>{}, itrToProp->fees, name(""), topropid, daterange, true, memo);
	}
}

ACTION wax_express_trade::creategift(name owner, const vector<nft_id_t>& nfts, const vector<asset_ex>& fts, name account_to, date_range daterange, string memo)
{
	createprop(owner, nfts, fts, vector<tuple<box_id_t, object_id_t, condition>>{}, exchange_fees{}, account_to, daterange, true, memo);
}

ACTION wax_express_trade::requestgift(name owner, const vector<tuple<box_id_t, object_id_t, condition>>& conditions, name account_to, date_range daterange, string memo)
{
	createprop(owner, vector<nft_id_t>{}, vector<asset_ex>{}, conditions, exchange_fees{}, account_to, daterange, true, memo);
}

ACTION wax_express_trade::acceptgift(name owner, uint64_t gift_id)
{
	acceptoffer(owner, gift_id);
}

ACTION wax_express_trade::sendgift(name owner, const vector<nft_id_t>& nfts, const vector<asset_ex>& fts, uint64_t gift_id, uint64_t box_id)
{
	require_auth(owner);
	const auto itrToProp = stproposals_.require_find(gift_id, string("Proposal gift_id = " + to_string(gift_id) + " does not exist").c_str());

	check(!(itrToProp->topropid != 0), "Proposal id = " + to_string(gift_id) + " is offer to topropid = " + to_string(itrToProp->topropid) + " .Cannot send gift to offer");
	check(!(itrToProp->owner == owner), "This proposal belong to you");
	check(!(itrToProp->toaccount != name("") && itrToProp->toaccount != owner), "This proposal is exclusively for account: " + itrToProp->toaccount.to_string());
	check(!(itrToProp->daterange.datestart > now()), "Proposal has start date. Time to wait: " + gettimeToWait(abs((int)((uint64_t)itrToProp->daterange.datestart - (uint64_t)now()))));
	check(!(itrToProp->daterange.dateexpire != 0 && itrToProp->daterange.dateexpire < now()), "Proposal is expired. It was expired: " + gettimeToWait(abs((int)((uint64_t)itrToProp->daterange.datestart - (uint64_t)now()))));

	check(!(try_accept_proposal(owner, gift_id, box_id, nfts, fts) == false), "Cannot send gift wrong nfts and fts for conditions at gift id =" + to_string(gift_id));

	acceptor_.remove_proposal_with_conditions(gift_id);
	acceptor_.remove_all_offers_for_proposal(gift_id);
}

ACTION wax_express_trade::createprop(name owner, const vector<nft_id_t>& nfts, const vector<asset_ex>& fts, const vector<tuple<box_id_t, object_id_t, condition>>& conditions, exchange_fees fees, name account_to, date_range daterange, bool auto_accept, string memo)
{
	require_auth(owner);

	if (is_account(account_to)) {
		require_recipient(account_to);
	}

	check(!(nfts.size() == 0 && fts.size() == 0 && conditions.size() == 0), must_be_at_least_one + warning_one_condition + "or" + errorEmptyFT_NFT);

	create_proposal(owner, nfts, fts, conditions, fees, account_to, 0, daterange, auto_accept, memo);
}

void wax_express_trade::create_proposal(name owner, const vector<nft_id_t>& nfts, const vector<asset_ex>& fts, const vector<tuple<box_id_t, object_id_t, condition>>& conditions, exchange_fees fees, name account_to, proposal_id_t topropid, date_range daterange, bool auto_accept, string memo)
{
	check(!(account_to != name("") && is_account(account_to) == false), "account_to is not an account");

	check(!(account_to == owner), "account_to cannot be equal to owner");

	check(!(memo.size() > MEMO_MAX_SIZE), "Parameter memo has more than " + to_string(MEMO_MAX_SIZE) + " bytes. You entered size = " + to_string(memo.size()));

	string dublicated_nft;
	check(!(checker.hasDublicatedNFT(nfts, dublicated_nft)), "nfts has dublicated id: " + dublicated_nft);

	string dublicated_ft;
	check(!(checker.hasDublicatedFT(fts, dublicated_ft)), "fts has dublicated item: " + dublicated_ft);

	map < tuple < box_id_t, object_id_t >, vector<condition>> mapconditions;

	for (auto i = 0; i < conditions.size(); i++)
	{
		const auto& condition = conditions[i];
		const auto&[boxid, objectid, one_condition] = condition;
		mapconditions[make_tuple(boxid, objectid)].push_back(one_condition);
	}

	const auto& isoffer = topropid != 0;
	// logic check for condition
	checker.syntaxis_checker(mapconditions, isoffer);

	const auto new_proposal_id = getid();

	create_conditions(owner, new_proposal_id, mapconditions);

	if (hasLogging) { print("\n new_proposal_id :" + to_string(new_proposal_id)); }

	name last_author = name("");
	uint64_t last_aid = 0;

	for (auto i = 0; i < nfts.size(); i++)
	{
		const auto& nft_id = nfts[i];
		const auto aid_index = sinventory_.template get_index<"aid"_n>();
		const auto itr_aid = aid_index.find(nft_id);

		check(!(itr_aid == aid_index.end()), "Wrong assetid: " + to_string(nft_id));
		check(!(itr_aid->aid != nft_id), "Wrong assetid: " + to_string(nft_id));
		check(!(itr_aid->owner != owner), "Wrong owner for assetid: " + to_string(nft_id) + ". You entered owner: " + owner.to_string() + " but asset belong to: " + itr_aid->owner.to_string());

		fees.checkfee(fee_processor.getAuthorFee(itr_aid->author));

		auto itr_seller_inventory = sinventory_.require_find(itr_aid->id, string("Inventory item id = " + to_string(itr_aid->id) + " does not exist").c_str());

		check(!(last_author != name("") && last_author != itr_seller_inventory->author), "Not allowed to create proposal with nfts from different authors. You have two different authors: " + last_author.to_string() + " for nft id: " + to_string(last_aid) + " and " + itr_seller_inventory->author.to_string() + " for nft id:" + to_string(itr_aid->id));
		last_author = itr_seller_inventory->author;
		last_aid = itr_aid->aid;

		check(!(itr_seller_inventory == sinventory_.end()), "Inventory id = " + to_string(itr_aid->id) + " does not exist");
		check(!(itr_seller_inventory->inproposal.size() > MAX_PROPOSALS_PER_NFT), "Inventory id = " + to_string(itr_seller_inventory->id) + " exide maximum proposals for one nft which is " + to_string(MAX_PROPOSALS_PER_NFT) + " proposals");

		sinventory_.modify(itr_seller_inventory, owner, [&](auto& s) {
			s.inproposal.push_back(new_proposal_id);
		});
	}

	for (auto j = 0; j < fts.size(); j++)
	{
		const auto ft_id = fts[j];
		const auto&[author_from_proposal, asset_from_proposal, assettype_from_proposal] = ft_id;
		const auto& owner_index = sinventory_.template get_index<"owner"_n>();

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
					+ ". For author: " + itr_byowner_inventory->author.to_string() + " asset: " + itr_byowner_inventory->quantity.to_string());

				if (hasLogging) {
					print("\n asset.amount: ", asset_from_proposal.amount);
					print("\n itr_byowner_inventory->quantity.amount: ", itr_byowner_inventory->quantity.amount);
				}

				check(!(asset_from_proposal.amount > itr_byowner_inventory->quantity.amount), "Not enough to create proposal with " + asset_from_proposal.to_string() + " from author: " + author_from_proposal.to_string() + " You have at balance: " + itr_byowner_inventory->quantity.to_string());

				auto itr_seller_inventory = sinventory_.find(itr_byowner_inventory->id);
				check(!(itr_seller_inventory == sinventory_.end()), "Inventory id = " + to_string(itr_seller_inventory->id) + " does not exist");
				check(!(itr_seller_inventory->inproposal.size() > MAX_PROPOSALS_PER_FT), "Inventory id = " + to_string(itr_seller_inventory->id) + " exide maximum proposals for one ft which is " + to_string(MAX_PROPOSALS_PER_FT) + " proposals");

				sinventory_.modify(itr_seller_inventory, owner, [&](auto& s) {
					s.inproposal.push_back(new_proposal_id);
				});

				found = true;
			}
		}

		check(found, "Asset was not found: assettype: " + to_string(assettype_from_proposal) + " author: " + author_from_proposal.to_string() + " symbol: " + asset_from_proposal.symbol.code().to_string());
	}

	stproposals_.emplace(get_self(), [&](auto& g) {
		g.id = new_proposal_id;
		g.owner = owner;
		g.nfts = nfts;
		g.fts = fts;
		g.fees = fees;
		g.topropid = topropid;
		g.toaccount = account_to;
		g.daterange = daterange;
		g.memo = memo;
		g.auto_accept = auto_accept;
	});
}

void wax_express_trade::accept_offer_or_gift(name owner, uint64_t offer_id, bool isgift)
{
	require_auth(owner);

	const auto itr_offer_to_accept = stproposals_.require_find(offer_id, string("Wrong offer id: " + to_string(offer_id)).c_str());

	if (!isgift)
	{
		const auto & is_offer = itr_offer_to_accept->topropid != 0;
		check(!(is_offer == false), "offer_id = " + to_string(offer_id) + " is proposal, use createoffer action to accept it");
	}

	check(!(itr_offer_to_accept->owner == owner), "This offer id = " + to_string(offer_id) + " belong to you");
	check(!(itr_offer_to_accept->daterange.datestart > now()), "Offer has start date. Time to wait: " + gettimeToWait(abs((int)((uint64_t)itr_offer_to_accept->daterange.datestart - (uint64_t)now()))));
	check(!(itr_offer_to_accept->daterange.dateexpire != 0 && itr_offer_to_accept->daterange.dateexpire < now()), "Offer is expired. It was expired: " + gettimeToWait(abs((int)((uint64_t)itr_offer_to_accept->daterange.datestart - (uint64_t)now()))));

	for (auto i = 0; i < itr_offer_to_accept->nfts.size(); i++)
	{
		const auto& nft_id = itr_offer_to_accept->nfts[i];

		const auto aid_index = sinventory_.template get_index<"aid"_n>();
		const auto itr_aid = aid_index.find(nft_id);

		string error = "nft id = " + to_string(nft_id) + " does not exist in inventory";
		check(!(itr_aid == aid_index.end()), error);
		check(!(itr_aid != aid_index.end() && itr_aid->aid != nft_id), error);

		if (hasLogging) { print("\n itr_aid->id: ", itr_aid->id); }
	}

	const auto& buyer_inventory_index = sinventory_.template get_index<"owner"_n>();

	for (auto itr_ft = itr_offer_to_accept->fts.begin(); itr_ft != itr_offer_to_accept->fts.end(); itr_ft++)
	{
		// loop over buyer inventory and check each item
		for (auto itr_buyer_inventory = buyer_inventory_index.find(owner.value); itr_buyer_inventory != buyer_inventory_index.end(); itr_buyer_inventory++)
		{
			if (owner.value != itr_buyer_inventory->owner.value) {
				break;
			}

			if (hasLogging) {
				print("\n itr_buyer_inventory->assettype: ", itr_buyer_inventory->assettype);
				print("\n itr_ft->assettype: ", itr_ft->assettype);
				print("\n itr_buyer_inventory->author: ", itr_buyer_inventory->author);
				print("\n itr_ft->author: ", itr_ft->author);
				print("\n itr_buyer_inventory->quantity: ", itr_buyer_inventory->quantity);
				print("\n itr_ft->quantity: ", itr_ft->quantity);
			}

			if (itr_buyer_inventory->assettype == itr_ft->assettype && itr_buyer_inventory->author == itr_ft->author && itr_buyer_inventory->quantity.symbol.code() == itr_ft->quantity.symbol.code())
			{
				check(!(itr_buyer_inventory->quantity.amount < itr_ft->quantity.amount),
					"Not enough amount for fungible token of author: " + itr_ft->author.to_string() + " asset: " + itr_ft->quantity.to_string() + " . Buyer " + owner.to_string() + " balance: " + itr_buyer_inventory->quantity.to_string());

				check(!(itr_buyer_inventory->quantity.symbol.precision() != itr_ft->quantity.symbol.precision()),
					"Wrong precision for ft parameter. Inventory id: " + to_string(itr_buyer_inventory->id) + ". Symbol: " + itr_buyer_inventory->quantity.symbol.code().to_string() +
					" has precision " + to_string(itr_buyer_inventory->quantity.symbol.precision()) + ". You entered precision: " +
					to_string(itr_ft->quantity.symbol.precision()));

				if (hasLogging) { print("\n itr_buyer_inventory->id: ", itr_buyer_inventory->id); }

				break;
			}
		}
	}

	const auto& payer = owner;

	const auto& seller = itr_offer_to_accept->owner;
	const auto& buyer = owner;

	const auto& seller_nfts = itr_offer_to_accept->nfts;
	const auto& seller_fts = itr_offer_to_accept->fts;

	if (isgift)
	{
		const auto& gifter = seller;
		const auto& gift_receiver = buyer;

		const auto& gift_nft = seller_nfts;
		const auto& gift_fts = seller_fts;

		if (hasLogging) { print("\n ------------ move_items gifter to caller of this function ------------ \n"); }
		acceptor_.move_items(payer, gifter, gift_receiver, gift_nft, gift_fts, exchange_fees{}, 0);
	}
	else
	{
		// read fts and nfts from proposal that connected with offer
		const auto itr_proposal_topropid = stproposals_.require_find(itr_offer_to_accept->topropid, string("Wrong proposal id: " + to_string(itr_offer_to_accept->topropid) + " that saved at topropid = " + to_string(offer_id)).c_str());
		const auto& buyer_nfts = itr_proposal_topropid->nfts;
		const auto& buyer_fts = itr_proposal_topropid->fts;

		// do exchange after the end of matching
		if (hasLogging) { print("\n ------------ move_items seller to buyer ------------ \n"); }
		acceptor_.move_items(payer, seller, buyer, seller_nfts, seller_fts, itr_proposal_topropid->fees, itr_proposal_topropid->id);

		if (hasLogging) { print("\n ------------ move_items buyer to seller ------------ \n"); }
		acceptor_.move_items(payer, buyer, seller, buyer_nfts, buyer_fts, exchange_fees{}, 0);
	}

	acceptor_.clean_proposals_offers_conditions();
}

ACTION wax_express_trade::acceptoffer(name owner, uint64_t offer_id)
{
	const auto itrOffer = stproposals_.require_find(offer_id, string("Offer id = " + to_string(offer_id) + " does not exist").c_str());

	if (hasLogging) {
		print("is_proposal_gift: ", acceptor_.is_proposal_gift(offer_id, itrOffer->topropid), " offer_id = ", offer_id, " itrOffer->topropid: ", itrOffer->topropid);
	}

	accept_offer_or_gift(owner, offer_id, acceptor_.is_proposal_gift(offer_id, itrOffer->topropid));
}

ACTION wax_express_trade::cancelprop(name owner, uint64_t proposal_id)
{
	require_auth(owner);

	const auto& itr = stproposals_.require_find(proposal_id, string("Proposal id: " + to_string(proposal_id) + " does not exist").c_str());
	name proposal_owner = owner;

	if (owner == itr->toaccount)
	{
		proposal_owner = itr->toaccount;
	}
	else
	{
		check(!(itr->owner != owner), "Wrong owner for proposal id: " + to_string(proposal_id) + ". You entered owner: " + owner.to_string() + " but proposal belong to: " + itr->owner.to_string());
	}

	const auto& owner_index = sinventory_.template get_index<"owner"_n>();

	for (auto itrowner = owner_index.find(proposal_owner.value); itrowner != owner_index.end(); itrowner++)
	{
		if (proposal_owner.value != itrowner->owner.value) {
			break;
		}

		auto inproposal = itrowner->inproposal;
		auto inprop_itr = find(inproposal.begin(), inproposal.end(), proposal_id);

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

ACTION wax_express_trade::depositprep(name owner, vector<nft_id_t>& nfts, vector<asset_ex>& fts)
{
	require_auth(owner);
	require_recipient(owner);
	check(!(nfts.size() == 0 && fts.size() == 0), must_be_at_least_one + errorEmptyFT_NFT);

	tables::sassets assets_owner(SIMPLEASSETS_CONTRACT, owner.value);

	for (auto i = 0; i < nfts.size(); i++)
	{
		const auto& nft_id = nfts[i];

		const auto& idxk = assets_owner.require_find(nft_id, string("Asset id: " + to_string(nft_id) + " was not found").c_str());
		const auto aid_index = sinventory_.template get_index<"aid"_n>();
		const auto itr_aid = aid_index.find(nft_id);

		if (itr_aid == aid_index.end() || (itr_aid != aid_index.end() && itr_aid->aid != nft_id))
		{
			const auto & id = getid();
			if (hasLogging) {
				print("sinventory_.emplace nft_id: ", nft_id, " inventory id: ", id);
			}

			sinventory_.emplace(get_self(), [&](auto& g) {
				g.id = id;
				g.owner = owner;
				g.aid = nft_id;
				g.assettype = INACTIVE_OFFSET + SA_NFT; // NFT
				g.author = idxk->author;
			});
		}
		else
		{
			if (hasLogging) {
				print("Asset ", nft_id, " already exist in inventory", itr_aid->id);
			}
		}
	}

	const auto& myinventory_index = sinventory_.template get_index<"owner"_n>();

	for (auto ift = 0; ift < fts.size(); ift++)
	{
		const auto& author = fts[ift].author;
		auto quantity = fts[ift].quantity;
		const auto& assettype = fts[ift].assettype;

		auto newasset = true;
		for (auto itrmynventory = myinventory_index.find(owner.value); itrmynventory != myinventory_index.end(); itrmynventory++)
		{
			if (owner.value != itrmynventory->owner.value) {
				break;
			}

			if (itrmynventory->author == author && itrmynventory->quantity.symbol.code() == quantity.symbol.code())
			{
				newasset = false;
				break;
			}
		}

		if (hasLogging)
		{
			if (!newasset)
			{
				print("\n asset author: ", author.to_string(), " assettype: ", assettype, " quantity: ", quantity, " already exist ");
			}
		}

		if (newasset)
		{
			quantity.amount = 0;

			const auto & id = getid();
			if (hasLogging) {
				print("sinventory_.emplace quantity: ", quantity, " inventory id: ", id);
			}

			sinventory_.emplace(owner, [&](auto& g) {
				g.id = id;
				g.owner = owner;
				g.assettype = INACTIVE_OFFSET + assettype;
				g.author = author;
				g.quantity = quantity;
			});
		}
	}
}

ACTION wax_express_trade::withdraw(name owner, vector<nft_id_t>& nfts, vector<asset_ex>& fts)
{
	require_auth(owner);
	require_recipient(owner);

	check(!(nfts.size() == 0 && fts.size() == 0), must_be_at_least_one + errorEmptyFT_NFT);

	for (auto i = 0; i < nfts.size(); i++)
	{
		const auto& nft_id = nfts[i];
		auto aid_index = sinventory_.template get_index<"aid"_n>();

		const auto& itr_aid = aid_index.require_find(nft_id, string("Asset does not exist in inventory. Wrong assetid: " + to_string(nft_id)).c_str());

		check(!(itr_aid->aid != nft_id), "Wrong assetid: " + to_string(nft_id));
		check(!(itr_aid->owner != owner), "Wrong owner for assetid: " + to_string(nft_id) + ". You entered owner: " + owner.to_string() + " but asset belong to: " + itr_aid->owner.to_string());

		acceptor_.remove_all_proposals_for_inventory_item(itr_aid->id);
		aid_index.erase(itr_aid);
	}

	if (nfts.size() > 0)
	{
		action(
			permission_level{ get_self(), "active"_n },
			SIMPLEASSETS_CONTRACT,
			"offer"_n,
			make_tuple(get_self(), owner, nfts, withdraw_memo)
		).send();
	}

	for (auto ift = 0; ift < fts.size(); ift++)
	{
		const auto& nft_id = fts[ift];
		auto myinventory_index = sinventory_.template get_index<"owner"_n>();

		const auto& withraw_author = fts[ift].author;
		const auto& withraw_asset = fts[ift].quantity;
		const uint64_t& assettype = fts[ift].assettype;

		for (auto itrmynventory = myinventory_index.find(owner.value); itrmynventory != myinventory_index.end(); itrmynventory++)
		{
			if (owner.value != itrmynventory->owner.value) {
				break;
			}

			if (itrmynventory->author == withraw_author && itrmynventory->quantity.symbol.code() == withraw_asset.symbol.code() && assettype == itrmynventory->assettype)
			{
				check(!(withraw_asset.amount > itrmynventory->quantity.amount), "Not enough to withdraw " + withraw_asset.to_string() + " from author: " + withraw_asset.to_string() + " You have ");

				if (withraw_asset.amount != itrmynventory->quantity.amount)
				{
					auto itr_my_invent = sinventory_.find(itrmynventory->id);
					if (itr_my_invent != sinventory_.end())
					{
						sinventory_.modify(itr_my_invent, owner, [&](auto& s) {
							s.quantity.amount -= withraw_asset.amount;
						});
					}

					if (!inventory_bank.canwithdraw(owner, itrmynventory->author, withraw_asset, assettype))
					{
						acceptor_.remove_all_proposals_for_inventory_item(itrmynventory->id);
					}
				}
				else
				{
					acceptor_.remove_all_proposals_for_inventory_item(itrmynventory->id);
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
						"offerf"_n,
						make_tuple(get_self(), owner, withraw_author, withraw_asset, withdraw_memo)
					).send();
				}
			}
		}
	}
}

void wax_express_trade::receiveASSETF(name from, name to, name author, asset quantity, string memo)
{
	if (from == get_self()) {
		return;
	}
	require_auth(from);

	check(!(sblacklist_.find(author.value) != sblacklist_.end()), "Author: " + author.to_string() + " was blacklisted");

	inventory_bank.depositFT(from, author, quantity, SA_FT);
}

void wax_express_trade::receiveASSET(name from, name to, vector<uint64_t>& assetids, string memo) {

	if (from == get_self()) {
		return;
	}

	require_auth(from);
	require_recipient(from);

	for (auto i = 0; i < assetids.size(); i++)
	{
		const auto nft_id = assetids[i];
		const auto& idxk = assets_.require_find(assetids[i], string("Asset id: " + to_string(assetids[i]) + " was not found").c_str());

		check(!(sblacklist_.find(idxk->author.value) != sblacklist_.end()), "Author: " + idxk->author.to_string() + " was blacklisted");

		const auto aid_index = sinventory_.template get_index<"aid"_n>();
		const auto itr_aid = aid_index.find(nft_id);

		const string errorMessage = "Must be called depositprep before sending asset. Wrong assetid: " + to_string(nft_id);
		check(!(itr_aid == aid_index.end()), errorMessage);
		check(!(itr_aid->aid != nft_id), errorMessage);

		check(!(itr_aid->assettype != SA_NFT + INACTIVE_OFFSET), "Inventory id: " + to_string(itr_aid->id) + " has assettype = " + to_string(itr_aid->assettype) + ", but must be in inactive state assettype = " + to_string(SA_NFT + INACTIVE_OFFSET));

		const auto & itr_inventory = sinventory_.find(itr_aid->id);

		check(!(itr_inventory == sinventory_.end()), errorMessage + " .id was not found");

		sinventory_.modify(itr_inventory, get_self(), [&](auto& s) {
			s.assettype = SA_NFT;
		});
	}
}

void wax_express_trade::receiveToken(name from, name to, asset quantity, string memo)
{
	if (to != get_self()) {
		return;
	}

	require_auth(from);

	inventory_bank.depositFT(from, name(wax_express_trade::code_test), quantity, TOKEN);
}

ACTION wax_express_trade::getbalance(name owner, name author, string sym)
{
	const auto& inventory_index = sinventory_.template get_index<"owner"_n>();

	for (auto itr_inventory = inventory_index.find(owner.value); itr_inventory != inventory_index.end(); itr_inventory++)
	{
		if (owner.value != itr_inventory->owner.value) {
			break;
		}

		if (itr_inventory->author == author && itr_inventory->quantity.symbol.code().to_string() == sym) {
			print("\n balance:", itr_inventory->quantity, ":");
		}
	}
}

uint64_t wax_express_trade::getid()
{
	conf config(_self, _self.value);
	_cstate = config.exists() ? config.get() : global{};
	_cstate.lastid++;
	config.set(_cstate, _self);

	return _cstate.lastid;
}


//#define DEBUG

#ifdef DEBUG

ACTION wax_express_trade::tstfee(name nft_author, exchange_fees fee, asset_ex payment_ft_from_proposal)
{
	fee.checkfee(fee_processor.getAuthorFee(nft_author));
	fee_processor.start(get_self(), nft_author, fee, payment_ft_from_proposal);

	const auto buyer_payment = fee_processor.getBuyerPart();
#ifdef AFFILIATEBANK
	if (const auto exchange_fee = fee_processor.getExchangeFee(); exchange_fee.quantity.amount > 0)
	{
		inventory_bank.depositFT(fee.exchange, exchange_fee.author, exchange_fee.quantity, exchange_fee.assettype);
	}
#endif

	if (const auto author_fee = fee_processor.getAuthorFee(); author_fee.quantity.amount > 0)
	{
		inventory_bank.depositFT(nft_author, author_fee.author, author_fee.quantity, author_fee.assettype);
	}
}

ACTION wax_express_trade::tstcondition(const vector<tuple<box_id_t, object_id_t, condition>>& conditions, bool isoffer)
{
	map < tuple < box_id_t, object_id_t>, vector<condition>> mapconditions;

	for (auto i = 0; i < conditions.size(); i++)
	{
		const auto& condition = conditions[i];

		const auto&[boxid, objectid, one_condition] = condition;

		mapconditions[make_tuple(boxid, objectid)].push_back(one_condition);
	}

	checker.syntaxis_checker(mapconditions, isoffer);
}

ACTION wax_express_trade::changetype(name owner, uint64_t inventory_id, uint64_t assettype)
{
	const auto& itr_inventory = sinventory_.find(inventory_id);
	check(!(itr_inventory == sinventory_.end()), "Inventory id " + to_string(inventory_id) + " does not exist");

	sinventory_.modify(itr_inventory, owner, [&](auto& s) {
		s.assettype = assettype;
	});
}

// erase proposals table
ACTION wax_express_trade::eraseallprop()
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

ACTION wax_express_trade::delproposal(name owner, uint64_t inventory_id)
{
	// delete all proposals and condition for inventory item
	require_auth(owner);

	// TABLE inventory{ 
	// find item in inventory table
	const auto& itr_inventory = sinventory_.find(inventory_id);
	check(!(itr_inventory == sinventory_.end()), "Inventory id " + to_string(inventory_id) + " does not exist");
	check(!(itr_inventory->owner != owner), owner.to_string() + " is not owner of inventory id: " + to_string(inventory_id) + ". Owner is " + itr_inventory->owner.to_string());

	acceptor_.remove_all_proposals_for_inventory_item(inventory_id);
}

ACTION wax_express_trade::delinventory(name owner, uint64_t inventory_id)
{
	require_auth(get_self());
	// delete all proposals and condition for inventory item
	//require_auth(owner);

	// TABLE inventory{ 
	// find item in inventory table
	const auto& itr_inventory = sinventory_.find(inventory_id);
	check(!(itr_inventory == sinventory_.end()), "Inventory id " + to_string(inventory_id) + " does not exist");
	//check(!(itr_inventory->owner != owner), owner.to_string() + " is not owner of inventory id: " + to_string(inventory_id) + ". Owner is " + itr_inventory->owner.to_string());

	acceptor_.remove_all_proposals_for_inventory_item(inventory_id);
	sinventory_.erase(itr_inventory);
}

ACTION wax_express_trade::testgetvalue(string filter, string imdata)
{
	// example of parameters:
	// filter = "mdata.cd.dd";
	// imdata = "{\"cd\":{\"dd\":10},\"health\":100,\"img\":\"https://k.cryptolions.io/0fde4208a0e20618d4968fdfbbafae25f88611d2f7178bbdc498f04a4c268dca.png\",\"kids\":0,\"name\":\"testkolob\",\"url\":\"simpleassets.io\"}";

	string result;
	const auto& res = acceptor_.get_value(filter, imdata, result);

	print("\n Function return: ", res, " result: ", result);
}

ACTION wax_express_trade::delcondition(name owner, uint64_t conditionid)
{
	require_auth(owner);

	const auto& itrcond = sconditions_.find(conditionid);

	check(!(itrcond == sconditions_.end()), "Wrong proposal id: " + to_string(conditionid));
	check(!(itrcond->owner != owner), "Wrong owner for condition id: " + to_string(conditionid) + ". You entered owner: " + owner.to_string() + " but condition belong to: " + itrcond->owner.to_string());

	sconditions_.erase(itrcond);
}

ACTION wax_express_trade::datamatch(uint64_t inventory_id, const condition& condition)
{
	//const auto& itr_inventory = sinventory_.find(inventory_id);
	//if (itr_inventory != sinventory_.end())
	//{
		//print("\n isdatamatch: " + to_string(acceptor_.isdatamatch(condition, itr_inventory->mdata)));
	//}
}

ACTION wax_express_trade::testisint(const string& value)
{
	print("\n isinteger(value):", isinteger(value));
}

ACTION wax_express_trade::tstwithdraw(name owner, name asset_author, asset withraw_asset, uint64_t assettype)
{
	print("\n canwithdraw(owner, asset_author, withraw_asset):", inventory_bank.canwithdraw(owner, asset_author, withraw_asset, assettype));
}

ACTION wax_express_trade::tstautfee(name author)
{
	print("\n getAuthorFee: ", fee_processor.getAuthorFee(author));
}

#endif