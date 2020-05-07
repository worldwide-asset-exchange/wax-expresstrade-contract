#pragma once
#include <eosio/eosio.hpp>
#include <string>
#include <tables.hpp>

using json = nlohmann::json;
using namespace eosio;
using namespace std;

struct inventory_bank
{
	tables::sinventory&  sinventory_;
	tables::stproposals& stproposals_;

	name self_;

	name get_self()
	{
		return self_;
	}

public:
	inventory_bank(tables::sinventory& inventory, tables::stproposals& stproposal, name self)
		: sinventory_(inventory), stproposals_(stproposal), self_(self)
	{

	}

	bool canwithdraw(name owner, name author, asset amount_to_withdraw, uint64_t assettype)
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
				const auto&[author_from_proposal, asst_from_proposal, assettype_from_proposal] = *itr;

				if (assettype_from_proposal == assettype && author_from_proposal == author && asst_from_proposal.symbol.code() == amount_to_withdraw.symbol.code() && asst_from_proposal.amount > amount_to_withdraw.amount)
				{
					result = false;
					break;
				}
			}
		}
		return result;
	}

	void depositFT(name deposit_account, name author, asset quantity, uint64_t assettype)
	{
		const auto& myinventory_index = sinventory_.template get_index<"owner"_n>();

		auto newasset = true;
		for (auto itrmynventory = myinventory_index.find(deposit_account.value); itrmynventory != myinventory_index.end(); itrmynventory++)
		{
			if (deposit_account.value != itrmynventory->owner.value) {
				break;
			}

			// do search if asset exist but not activated
			if (itrmynventory->author == author && itrmynventory->assettype == assettype + INACTIVE_OFFSET && itrmynventory->quantity.symbol.code() == quantity.symbol.code())
			{
				const auto& itr = sinventory_.find(itrmynventory->id);
				if (itr != sinventory_.end())
				{
					check(!(quantity.symbol.precision() != itrmynventory->quantity.symbol.precision()), "Incorrect precision. You entered quantity with precision: "
							+ to_string(quantity.symbol.precision()) + ". At inventory precision: " + to_string(itrmynventory->quantity.symbol.precision())
							+ ". For author: " + itrmynventory->author.to_string() + " asset: " + itrmynventory->quantity.to_string());

					// activate inactive asset and change balance
					sinventory_.modify(itr, get_self(), [&](auto& s) {
						s.assettype -= INACTIVE_OFFSET;
						s.quantity.amount += quantity.amount;
					});
					newasset = false;
				}
			}
			else if (itrmynventory->author == author && itrmynventory->assettype == assettype && itrmynventory->quantity.symbol.code() == quantity.symbol.code())
			{
				const auto& itr = sinventory_.find(itrmynventory->id);
				if (itr != sinventory_.end())
				{
					if (hasLogging) { print("\n Deposit of owner " + itr->owner.to_string() + " before modify itr->quantity:  ", itr->quantity); }

					check(!(quantity.symbol.precision() != itrmynventory->quantity.symbol.precision()), "Incorrect precision. You entered quantity with precision: "
						+ to_string(quantity.symbol.precision()) + ". At inventory precision: " + to_string(itrmynventory->quantity.symbol.precision())
						+ ". For author: " + itrmynventory->author.to_string() + " asset: " + itrmynventory->quantity.to_string());


					sinventory_.modify(itr, get_self(), [&](auto& s) {
						s.quantity.amount += quantity.amount;
					});

					if (hasLogging) { print("\n Deposit of owner " + itr->owner.to_string() + " after modify itr->quantity: ", itr->quantity); }

					newasset = false;
					break;
				}
			}
		}

		check(!(newasset == true),
			"Call once depositprep action before sending asset from author: " + author.to_string() + " assettype: " + to_string(assettype) + " quantity: " + quantity.to_string());
	}
};