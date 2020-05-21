#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/print.hpp>
#include <tuple>
#include <string>
#include <json.hpp>
#include <common.hpp>
#include <exchange_fees.hpp>

using json = nlohmann::json;
using namespace eosio;
using namespace std;

class fee_processor
{
public:

	TABLE sauthor{
		name			author;
		string			dappinfo;
		string			fieldtypes;
		string			priorityimg;

		auto primary_key() const {
			return author.value;
		}
	};
	typedef eosio::multi_index< "authors"_n, sauthor > authors;

private:
	asset_ex payment_part_to_exchange;
	asset_ex payment_part_to_affiliate;
	asset_ex payment_part_to_buyer;
	asset_ex payment_part_ft_to_author;
	asset_ex payment_part_ft_to_community;
public:

	asset_ex getAffiliateFee()
	{
		return payment_part_to_affiliate;
	}

	asset_ex getExchangeFee()
	{
		return payment_part_to_exchange;
	}

	asset_ex getBuyerPart()
	{
		return payment_part_to_buyer;
	}

	asset_ex getAuthorFee()
	{
		return payment_part_ft_to_author;
	}

	int64_t calculateCommunityAmountFee(const int64_t& author_amount)
	{
		return (int64_t)((((double)COMMUNITY_FEE / (double)FEE_PRECISION_AMOUNT) * (double)author_amount) / (double)PERCENT100);
	}

	int64_t calculateAffiliateAmountFee(const exchange_fees& fee, const int64_t& exchange_amount)
	{
		return (int64_t)((((double)fee.affiliate_fee / (double)FEE_PRECISION_AMOUNT) * (double)exchange_amount) / (double)PERCENT100);
	}

	int64_t calculateExchangeAmountFee(const exchange_fees& fee, const int64_t& original_amount)
	{
		if (hasLogging) { print("\n fee.exchange_fee: ", fee.exchange_fee); }

		const double result = ((((double)fee.exchange_fee / (double)FEE_PRECISION_AMOUNT) * (double)original_amount) / (double)PERCENT100);

		if (hasLogging) { print("\n ((((double)author_fee = ", (double)fee.exchange_fee, " / (double)FEE_PRECISION_AMOUNT = ", (double)FEE_PRECISION_AMOUNT, ") * (double)original_amount", (double)original_amount, ") / (double)PERCENT100) = ", PERCENT100); }

		return (int64_t)result;
	}

	int64_t calculateAuthorAmountFee(const name& author, const int64_t& original_amount)
	{
		const auto author_fee = getAuthorFee(author);
		if (hasLogging) { print("\n author_fee: ", author_fee); }
		const double result = ((((double)author_fee / (double)FEE_PRECISION_AMOUNT) * (double)original_amount) / (double)PERCENT100);

		if (hasLogging) { print("\n ((((double)author_fee = ", (double)author_fee, " / (double)FEE_PRECISION_AMOUNT = ", (double)FEE_PRECISION_AMOUNT, ") * (double)original_amount", (double)original_amount, ") / (double)PERCENT100) = ", PERCENT100); }

		return (int64_t)result;
	}

	uint16_t getAuthorFee(name author)
	{
		uint16_t result = 0;
		authors author_(SIMPLEASSETS_CONTRACT, SIMPLEASSETS_CONTRACT.value);

		if (auto itr = author_.find(author.value); itr != author_.end()) {

			if (json::accept(itr->dappinfo)) {

				auto dappinfojson = json::parse(itr->dappinfo);

				if (dappinfojson.dump() != "<discarded>" && !dappinfojson["defaultfee"].is_null()) {
					result = (uint16_t)dappinfojson["defaultfee"];
				}
			}
		}

		if (result > AUTHOR_FEE_MAX)
		{
			if (hasLogging) {
				print("\n Author " + author.to_string() + " entered wrong fee =" + to_string(result) + " in Simple asset`s " + to_string(result) + " Author`s fee cannot be bigger then " + to_string(AUTHOR_FEE_MAX));
			}
			result = AUTHOR_FEE_MAX;
		}

		return result;
	}

	void start(name payer_name, name nft_author, exchange_fees fee, asset_ex payment_ft_from_proposal)
	{
#ifdef AFFILIATEBANK
		payment_part_to_exchange = payment_ft_from_proposal;
		payment_part_to_affiliate = payment_ft_from_proposal;
#endif
		payment_part_to_buyer = payment_ft_from_proposal;
		payment_part_ft_to_author = payment_ft_from_proposal;
		payment_part_ft_to_community = payment_ft_from_proposal;

		// author + community fee processing
		const auto amount_for_author = calculateAuthorAmountFee(nft_author, payment_ft_from_proposal.quantity.amount);
		payment_part_to_buyer.quantity.amount -= amount_for_author;

		const auto amount_for_community = calculateCommunityAmountFee(amount_for_author);
		payment_part_ft_to_community.quantity.amount = amount_for_community;
		payment_part_ft_to_author.quantity.amount = amount_for_author - amount_for_community;

		// Exchange + Affiliates fee processing
		const auto amount_for_exchange = calculateExchangeAmountFee(fee, payment_ft_from_proposal.quantity.amount);
#ifdef AFFILIATEBANK
		payment_part_to_buyer.quantity.amount -= amount_for_exchange;
		const auto amount_for_affiliate = calculateAffiliateAmountFee(fee, amount_for_exchange);
		payment_part_to_affiliate.quantity.amount = amount_for_affiliate;
		payment_part_to_exchange.quantity.amount = amount_for_exchange - amount_for_affiliate; */
#endif

			if (hasLogging) {
				print("\n payment_ft_from_proposal.assettype: ", payment_ft_from_proposal.assettype);
				print("\n payment_part_to_exchange.quantity:  ", payment_part_to_exchange.quantity);
				print("\n payment_part_to_affiliate.quantity: ", payment_part_to_affiliate.quantity);
				print("\n payment_part_to_buyer.quantity:     ", payment_part_to_buyer.quantity);
				print("\n payment_part_ft_to_author.quantity: ", payment_part_ft_to_author.quantity);
			}

		if (amount_for_community > 0)
		{
			sendFeeToCommunity(payer_name, payment_part_ft_to_community);
		}

#ifdef AFFILIATEBANK
		if (amount_for_affiliate > 0)
		{
			sendFeeToAffiliateBank(payer_name, fee, payment_part_to_affiliate);
		}
#endif
	}

	void sendFeeToCommunity(name payer_name, asset_ex payment_to_community)
	{
		string fee_memo = "Community fee";
		if (payment_to_community.assettype == TOKEN)
		{
			action(
				permission_level{ payer_name,"active"_n },
				payment_to_community.author, // eosio.token for WAX
				"transfer"_n,
				make_tuple(payer_name, COMMUNITY_FEE_ACCOUNT, payment_to_community.quantity, fee_memo)
			).send();
		}
		else if (payment_to_community.assettype == SA_FT)
		{
			action(
				permission_level{ payer_name,"active"_n },
				SIMPLEASSETS_CONTRACT,
				"transferf"_n,
				make_tuple(payer_name, COMMUNITY_FEE_ACCOUNT, payment_to_community.author, payment_to_community.quantity, fee_memo)
			).send();
		}
		else
		{
			check(false, "Wrong assettype. You entered assettype = " + to_string(payment_to_community.assettype));
		}
	}

	void sendFeeToAffiliateBank(name payer_name, exchange_fees fee, asset_ex payment_to_exchange)
	{
		json fee_memo;
		fee_memo["exchange"] = fee.exchange.to_string();
		fee_memo["exchange_fee"] = fee.exchange_fee;
		fee_memo["affiliate_id"] = fee.affiliate_id;
		fee_memo["affiliate_fee"] = fee.affiliate_fee;

		if (hasLogging) {
			print("\n fee_memo: ", fee_memo.dump());
			print("\n payment_to_exchange.author: ", payment_to_exchange.author, " payment_to_exchange.quantity: ", payment_to_exchange.quantity, " payment_to_exchange.assettype: ", payment_to_exchange.assettype);
		}

		if (payment_to_exchange.assettype == TOKEN)
		{
			action(
				permission_level{ payer_name,"active"_n },
				payment_to_exchange.author, // eosio.token for EOS
				"transfer"_n,
				make_tuple(payer_name, AFFILIATE_BANK, payment_to_exchange.quantity, fee_memo.dump())
			).send();
		}
		else if (payment_to_exchange.assettype == SA_FT)
		{
			action(
				permission_level{ payer_name,"active"_n },
				SIMPLEASSETS_CONTRACT,
				"transferf"_n,
				make_tuple(payer_name, AFFILIATE_BANK, payment_to_exchange.author, payment_to_exchange.quantity, fee_memo.dump())
			).send();
		}
		else
		{
			check(false, "Wrong assettype. You entered assettype = " + to_string(payment_to_exchange.assettype));
		}
	}

	void sendFeeToAuthor(name payer_name, name nft_author, asset_ex payment_to_author)
	{
		if (hasLogging) {
			print("\n payment_to_author.author: ", payment_to_author.author, " payment_to_author.quantity: ", payment_to_author.quantity, " payment_to_author.assettype: ", payment_to_author.assettype);
		}

		string fee_memo = "Author`s fee";
		if (payment_to_author.assettype == TOKEN)
		{
			action(
				permission_level{ payer_name, "active"_n },
				payment_to_author.author, // eosio.token for EOS
				"transfer"_n,
				make_tuple(payer_name, nft_author, payment_to_author.quantity, fee_memo)
			).send();
		}
		else if (payment_to_author.assettype == SA_FT)
		{
			action(
				permission_level{ payer_name, "active"_n },
				SIMPLEASSETS_CONTRACT,
				"transferf"_n,
				make_tuple(payer_name, nft_author, payment_to_author.author, payment_to_author.quantity, fee_memo)
			).send();
		}
		else
		{
			check(false, "Wrong assettype. You entered assettype = " + to_string(payment_to_author.assettype));
		}
	}

};