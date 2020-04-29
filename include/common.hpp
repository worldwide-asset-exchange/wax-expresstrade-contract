#pragma once

#include <eosio/eosio.hpp>
#include <map>
#include <tuple>
#include <vector>
#include <string>

using namespace eosio;
using namespace std;

//#define DEBUG
//#define EOS_CHAIN
#define WAX_CHAIN
bool hasLogging = true;

const name SIMPLEASSETS_CONTRACT = "simpleassets"_n;
const name EOSIO_TOKEN = "eosio.token"_n;
const name AFFILIATE_BANK = "affiliateban"_n;
const name COMMUNITY_FEE_ACCOUNT = "communityfee"_n;

enum token_type { SA_NFT = 0, SA_FT = 1, TOKEN = 2 };

enum proposal_type { proposal = 0, offer = 1 };

enum match { no_need_match = -1, failed_match = 0, has_match = 1 };

typedef uint64_t box_id_t;
typedef uint64_t object_id_t;
typedef uint64_t assettype_t;
typedef uint64_t nft_id_t;
typedef uint64_t inventory_id_t;
typedef string   key_t;
typedef string   operation_t;
typedef string   value_t;
typedef uint64_t amount_t;
typedef uint64_t proposal_id_t;

const uint8_t  FEE_PRECISION = 2;
const uint16_t FEE_PRECISION_AMOUNT = 100;
const uint16_t FEE_MAX = 10 * FEE_PRECISION_AMOUNT;
const uint16_t AUTHOR_FEE_MAX = 5 * FEE_PRECISION_AMOUNT;

const uint16_t COMMUNITY_FEE = 10 * FEE_PRECISION_AMOUNT;
const uint8_t  PERCENT100 = 100;
const uint16_t MEMO_MAX_SIZE = 256;
const uint64_t INACTIVE_OFFSET = 1000000;
const string   authorFeeTagName = "defaultfee";
const uint64_t MAX_PROPOSALS_PER_NFT = 20;
const uint64_t MAX_PROPOSALS_PER_FT = 40;
const uint8_t  FIRST_INDEX = 1;

#ifdef WAX_CHAIN
const string SYMBOL_NAME = "WAX";
const int PRECISION = 8;
#else
#ifdef EOS_CHAIN
const string SYMBOL_NAME = "EOS";
const int PRECISION = 4;
#endif
#endif

struct condition
{
	string key;
	string operation;
	string value;
};

struct asset_ex
{
	name        author;
	asset       quantity;
	assettype_t assettype;
};

inline uint32_t now() {
	static uint32_t current_time = current_time_point().sec_since_epoch();
	return current_time;
}

bool isinteger(const string &str) {
	return !str.empty() && find_if(str.begin(), str.end(), [](char chr) { return !isdigit(chr); }) == str.end();
}

string gettimeToWait(uint64_t time_in_seconds)
{
	uint64_t s, h, m = 0;
	m = time_in_seconds / 60;
	h = m / 60;
	return to_string(int(h)) + " hours " + to_string(int(m % 60)) + " minutes " + to_string(int(time_in_seconds % 60)) + " seconds";
}

const vector<string> explode(const string& s, const char& c)
{
	string buff;
	vector<string> v;

	for (const auto& n : s) {
		if (n != c) {
			buff += n;
		}
		else {
			if (n == c && buff != "") {
				v.push_back(buff);
				buff = "";
			}
		}
	}

	if (!buff.empty()) {
		v.push_back(buff);
	}

	return v;
}

int precision_from_string(const string& str)
{
	auto result = 0;
	auto start = str.find(".");

	if (start != string::npos)
	{
		auto end = str.find(" ", start);
		result = end - start - 1;
	}
	return result;
}

asset asset_from_string(const string& s)
{
	const auto precision = precision_from_string(s);
	vector<string> v = explode(s, ' ');
	vector<string> v1 = explode(v[0], '.');

	for (auto i = v1[1].length(); i < precision; i++) {
		v1[1] += "0";
	}

	// asset( amount, symbol );
	return asset(stoull(v1[0] + v1[1]), symbol(symbol_code(v[1]), precision));
}

void split(const string& subject, vector<string>& container)
{
	container.clear();
	size_t len = subject.length() + 1;
	char* s = new char[len];
	memset(s, 0, len * sizeof(char));
	memcpy(s, subject.c_str(), (len - 1) * sizeof(char));
	for (char *p = strtok(s, "."); p != NULL; p = strtok(NULL, "."))
	{
		container.push_back(p);
	}
	delete[] s;
}

auto find_condition(const vector<condition>& aconditions, const string& condition_key)
{
	const auto& it = find_if(aconditions.begin(), aconditions.end(),
		[&](const condition& one_condition) { return one_condition.key == condition_key; });

	return (it != aconditions.end()) ? optional<condition>(*it) : std::nullopt;
}


