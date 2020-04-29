#pragma once
#include <eosio/eosio.hpp>
#include <string>
#include <common.hpp>
using namespace eosio;
using namespace std;


struct exchange_fees
{
	name     exchange;
	uint16_t exchange_fee;
	uint64_t affiliate_id;
	uint16_t affiliate_fee;

	void checkfee(uint16_t author_fee)
	{
		check(!(affiliate_fee > PERCENT100 * FEE_PRECISION_AMOUNT), "affiliate_fee cannot be bigger then 100% which is " + to_string(PERCENT100 * FEE_PRECISION_AMOUNT) + ". You entered affiliate_fee = " + to_string(affiliate_fee));
		check(!(author_fee + exchange_fee > FEE_MAX), "Author Fee with Exchange fee cannot be bigger then " + to_string(FEE_MAX / FEE_PRECISION_AMOUNT) + "%. You entered: " + to_string((author_fee + exchange_fee) / FEE_PRECISION_AMOUNT) + "%");
	}
};
