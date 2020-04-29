#include <AffiliateBank.hpp>

ACTION AffiliateBank::addexchange(const name& exchange, const vector<key_value>& data) {
	require_auth(exchange);

	check(!(sexchanges_.find(exchange.value) != sexchanges_.end()), "Exchange  " + exchange.to_string() + " already exist");

	sexchanges_.emplace(exchange, [&](auto& g) {
		g.exchange = exchange;
		g.data     = data;
	});
}

ACTION AffiliateBank::addaffiliate(const name& exchange, const name& affiliate, const string& affiliate_name, const vector<key_value>& data) {
	require_auth(exchange);

	check(is_account(affiliate), affiliate.to_string() + " is not an account");
	check(!(sexchanges_.find(exchange.value) == sexchanges_.end()), "Exchange " + exchange.to_string() + " does not exist");

	saffiliates_.emplace(exchange, [&](auto& g) {
		g.id             = saffiliates_.available_primary_key();
		g.exchange       = exchange;
		g.affiliate      = affiliate;
		g.affiliate_name = affiliate_name;
		g.data           = data;
	});
}

ACTION AffiliateBank::modexchange(const name& exchange, const vector<key_value>& data) {
	require_auth(exchange);

	const auto itr = sexchanges_.require_find(exchange.value, ((string)("Exchange  " + exchange.to_string() + " does not exist")).c_str());

	sexchanges_.modify(itr, exchange, [&](auto& g) {
		g.exchange = exchange;
		g.data     = data;
	});
}

ACTION AffiliateBank::modaffiliate(const uint64_t& affiliate_id, const name& exchange, const name& affiliate, const string& affiliate_name, const vector<key_value>& data) {
	require_auth(exchange);

	check(is_account(affiliate), affiliate.to_string() + " is not an account");
	
	const auto itr = saffiliates_.require_find(affiliate_id, ((string)("Affiliate id: " + to_string(affiliate_id) + " does not exist)")).c_str());

	check(!(sexchanges_.find(exchange.value) == sexchanges_.end()), "Exchange " + exchange.to_string() + " does not exist");

	saffiliates_.modify(itr, exchange, [&](auto& g) {
		g.exchange       = exchange;
		g.affiliate      = affiliate;
		g.affiliate_name = affiliate_name;
		g.data           = data;
	});
}

ACTION AffiliateBank::withdraw(const uint64_t & affiliate_id, const name& affiliate, vector<asset_ex>& fts)
{
	require_auth(affiliate);
	check(!(fts.size() == 0), "Must be at least one fungible token");

	const auto& itr_affiliate = saffiliates_.find(affiliate_id);
	check(!(itr_affiliate == saffiliates_.end()), "Cannot find affiliate id: " + to_string(affiliate_id));
	check(!(itr_affiliate->affiliate != affiliate), 
		"affiliate_id: " + to_string(affiliate_id) +  " has affiliate: " + itr_affiliate->affiliate.to_string() + " but you entered: " + affiliate.to_string());

	auto balance = itr_affiliate->balance;

	for (auto i = 0; i < fts.size(); ++i)
	{
		const auto ft_to_withdraw = fts[i];
		auto it = find_if(balance.begin(), balance.end(),
			[&](const asset_ex& one_asset) {
			return one_asset.author                 == ft_to_withdraw.author
				&& one_asset.assettype              == ft_to_withdraw.assettype
				&& one_asset.quantity.symbol.code() == ft_to_withdraw.quantity.symbol.code(); });

		check(!(it == balance.end()), 
			"Cannot withdraw ft for author: " + ft_to_withdraw.author.to_string() + " assettype: " 
			+ to_string(ft_to_withdraw.assettype) + "symbol: " + ft_to_withdraw.quantity.symbol.code().to_string());
		
		check(!(it->quantity.amount < ft_to_withdraw.quantity.amount), "Balance overdrawn. Balance: " + it->quantity.to_string() + ". You tried to withdraw: " + ft_to_withdraw.quantity.to_string());
		
		it->quantity.amount -= ft_to_withdraw.quantity.amount;
	}

	saffiliates_.modify(itr_affiliate, get_self(), [&](auto& s) {
		s.balance = balance;
	});
}

void AffiliateBank::depositFT(exchange_fees fee, asset_ex payment)
{
	const auto& itr_affiliate = saffiliates_.find(fee.affiliate_id);
	check(!(itr_affiliate == saffiliates_.end()), "Cannot find affiliate id: " + to_string(fee.affiliate_id));

	check(!(sexchanges_.find(fee.exchange.value) == sexchanges_.end()), "Cannot find exchange:  " + fee.exchange.to_string());

	auto balance = itr_affiliate->balance;

	auto it = find_if(balance.begin(), balance.end(),
		[&](const asset_ex& one_asset) { 
		return one_asset.author                 == payment.author
			&& one_asset.assettype              == payment.assettype
			&& one_asset.quantity.symbol.code() == payment.quantity.symbol.code(); });

	if (it != balance.end())
	{
		it->quantity.amount += payment.quantity.amount;
		saffiliates_.modify(itr_affiliate, get_self(), [&](auto& s) {
			s.balance = balance;
		});
	}
	else
	{
		saffiliates_.modify(itr_affiliate, get_self(), [&](auto& s) {
			s.balance.emplace_back(payment);
		});
	}
}

void AffiliateBank::receiveASSETF(name from, name to, name author, asset quantity, string memo)
{
	check(!(from != WAX_EXPRESS_TRADE), errorWAXTradeOnly);
	require_auth(from);
	
	depositFT(parseMemo(memo), { author, quantity, SA_FT });
}

AffiliateBank::exchange_fees AffiliateBank::parseMemo(const string& memo)
{
	exchange_fees result;
	check(json::accept(memo), "memo must be in JSON");
	auto json_fee = json::parse(memo, nullptr, false);

	// unable to parse json
	if (json_fee.dump() == "<discarded>")
	{
		check(false, "json cannot be parsed. " + memo);
	}

	result.exchange	     = name((string)json_fee["exchange"]);
	result.exchange_fee  = (uint16_t)json_fee["exchange_fee"];
	result.affiliate_id  = (uint64_t)json_fee["affiliate_id"];
	result.affiliate_fee = (uint64_t)json_fee["affiliate_fee"];

	check(is_account(result.exchange), result.exchange.to_string() + " is not an account");

	return result;
}

void AffiliateBank::receiveToken(name from, name to, asset quantity, string memo)
{
	check(!(from != WAX_EXPRESS_TRADE), errorWAXTradeOnly);
	require_auth(from);
	depositFT(parseMemo(memo), { name(AffiliateBank::code_test), quantity, TOKEN });
}
