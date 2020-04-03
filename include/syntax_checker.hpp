#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/transaction.hpp>
#include <eosio/crypto.hpp>
#include <eosio/print.hpp>
#include <common.hpp>

#include <map>
#include <tuple>
#include <vector>
#include <string>

using namespace eosio;
using namespace std;

class SyntaxChecker
{
public:

	bool hasDublicatedFT(const vector<tuple<name, asset, assettype_t>>& fts, string& out_dublicated_ft)
	{
		bool result = false;
		set<tuple<name, asset, uint64_t>> check_dublicated_ft_set;

		for (auto i = 0; i < fts.size(); i++)
		{
			const auto& ft = fts[i];

			auto ret = check_dublicated_ft_set.insert(ft);
			if (ret.second == false)
			{
				out_dublicated_ft = get<0>(ft).to_string() + " " + get<1>(ft).to_string() + " " + to_string(get<2>(ft));
				result = true;
				break;
			}
		}

		return result;
	}

	bool hasDublicatedNFT(const vector<uint64_t>& nfts, string& out_dublicated_nft)
	{
		bool result = false;
		set<uint64_t> check_dublicate_nft_set;

		for (auto i = 0; i < nfts.size(); i++)
		{
			const auto& nft_id = nfts[i];

			auto ret = check_dublicate_nft_set.insert(nft_id);
			if (ret.second == false)
			{
				out_dublicated_nft = to_string(nft_id);
				result = true;
				break;
			}
		}

		return result;
	}


	void check_key(const box_id_t & box_id, const object_id_t & object_id, const tuple<key_t, operation_t, value_t>& onecondition)
	{
		const auto&[condition_key, condition_operation, condition_value] = onecondition;

		check(!(!(condition_key == "author" || condition_key == "category" || condition_key == "assettype" || condition_key == "owner" || condition_key == "quantity" ||
			condition_key == "id" || condition_key.find("mdata") != string::npos || condition_key.find("idata") != string::npos) == true), "Condition key must be one of :( author, category, assettype, quantity ,owner, id , mdata, idata ). You entered: [" + condition_key + " " + condition_operation + " " + condition_value + "]. At box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));
	}

	void check_value(const box_id_t & box_id, const object_id_t & object_id, const tuple<key_t, operation_t, value_t>& onecondition)
	{
		const auto&[condition_key, condition_operation, condition_value] = onecondition;

		if (condition_key == "author")
		{
			check(!(is_account(name(condition_value)) == false), "Author`s name: " + condition_value + " is not an account at box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));
		}
		else if (condition_key == "assettype")
		{
			const auto assettype_condition_value = atoi(condition_value.c_str());

			// check is assettype valid
			check(!(!(assettype_condition_value == SA_NFT || assettype_condition_value == SA_FT || assettype_condition_value == TOKEN) == true || isinteger(condition_value) == false),
				"assettype must be equal: 0(SA_NFT) or 1(SA_FT) or 2(TOKEN). You entered: assettype = " + condition_value + " . At box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));
		}
		else if (condition_key == "id")
		{
			check(!(isinteger(condition_value) == false), "id condition must be integer value . You entered id = " + condition_value + " . At box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));
		}
	}

	void check_operation(const box_id_t & box_id, const object_id_t & object_id, const tuple<key_t, operation_t, value_t>& onecondition)
	{
		const auto&[condition_key, condition_operation, condition_value] = onecondition;

		check(!(!(condition_operation == "=" || condition_operation == "!=" || condition_operation == ">" || condition_operation == "<" ||
			condition_operation == "<=" || condition_operation == ">=") == true), "Condition operation must be one of :( =, != , <, > , <=, >= ). You entered: [" + condition_key + " " + condition_operation + " " + condition_value + "]. At box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));

		if (condition_key == "author")
		{
			if (condition_operation == "=")
			{
				check(!(is_account(name(condition_value)) == false), "Author`s name: " + condition_value + " is not an account at box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));
			}
			else
			{
				check(false, "Condition for author must be only '= operator' at box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));
			}
		}
		else if (condition_key == "assettype")
		{
			if (condition_operation != "=")
			{
				check(false, "Condition for assettype must be only '= operator' at box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));
			}
		}
		else if (condition_key == "category")
		{
			if (condition_operation != "=")
			{
				check(false, "Condition for category must be only '= operator' at box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));
			}
		}
		else if (condition_key == "owner")
		{
			if (condition_operation == "=")
			{
				check(!(is_account(name(condition_value)) == false), "Owner`s name: " + condition_value + " is not an account at box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));
			}
			else
			{
				check(false, "Condition for owner must be only '= operator' at box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));
			}
		}
		else if (condition_key == "id")
		{
			if (condition_operation != "=")
			{
				check(false, "Condition for id must be only '= operator' at box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));
			}
		}
	}

	auto find_condition(const vector<tuple<key_t, operation_t, value_t>>& aconditions, const string& condition_key)
	{
		const auto& it = find_if(aconditions.begin(), aconditions.end(),
			[&](const tuple<key_t, operation_t, value_t>& one_condition) { return get<0>(one_condition) == condition_key; });

		return (it != aconditions.end()) ? optional<tuple<key_t, operation_t, value_t>>(*it) : std::nullopt;
	}

	int count_of(const vector<tuple<key_t, operation_t, value_t>> & aconditions, const string &condition_name)
	{
		return count_if(aconditions.begin(), aconditions.end(), [&](tuple<key_t, operation_t, value_t> one_condition) { return get<0>(one_condition) == condition_name; });
	}

	int count_of_data(const vector<tuple<key_t, operation_t, value_t>> & aconditions, const string &condition_name)
	{
		return count_if(aconditions.begin(), aconditions.end(), [&](tuple<key_t, operation_t, value_t> one_condition) { return get<0>(one_condition).find(condition_name) == 0; });
	}

	void check_duplication(const unsigned & count, const string &count_name, const unsigned &box_id, const unsigned& object_id)
	{
		check(!(count > 1), "More then one condition " + count_name + " for one object. Box id: " + to_string(box_id) + " object_id: " + to_string(object_id));
	}

	void syntaxis_checker(const map < tuple < box_id_t, object_id_t>, vector<tuple<key_t, operation_t, value_t>>> & mapconditions)
	{
		map <box_id_t, vector<tuple<name, asset, assettype_t>>> ft_tokens;
		map <box_id_t, vector<nft_id_t>>                        nft_tokens;

		for (auto itr_map = mapconditions.begin(); itr_map != mapconditions.end(); itr_map++)
		{
			const auto &[box_id, object_id] = itr_map->first;
			const auto &aconditions = itr_map->second;

			const auto count_author = count_of(aconditions, "author");
			const auto count_assettype = count_of(aconditions, "assettype");
			const auto count_owner = count_of(aconditions, "owner");
			const auto count_id = count_of(aconditions, "id");
			const auto count_category = count_of(aconditions, "category");
			const auto count_quantity = count_of(aconditions, "quantity");
			const auto count_mdata = count_of_data(aconditions, "mdata");
			const auto count_idata = count_of_data(aconditions, "idata");

			// only one instance of following conditions allowed for one object
			check_duplication(count_author, "author", box_id, object_id);
			check_duplication(count_quantity, "quantity", box_id, object_id);
			check_duplication(count_assettype, "assettype", box_id, object_id);
			check_duplication(count_owner, "owner", box_id, object_id);
			check_duplication(count_id, "id", box_id, object_id);
			check_duplication(count_category, "category", box_id, object_id);
			check_duplication(count_mdata, "mdata", box_id, object_id);
			check_duplication(count_idata, "idata", box_id, object_id);

			check(!(count_assettype == 0), "Must be added condition with assettype. At box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));

			const auto assettype_condition = find_condition(aconditions, "assettype");
			const auto assettype_condition_value = atoi(((string)get<2>(*assettype_condition)).c_str());

			const auto hasIDCondition = (count_id > 0);
			// check will work only if condition no have 'id'. For 'id' it no need 
			if (!hasIDCondition) {
				check(!(count_author == 0), "Must be added condition with author. At box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));
			}

			// validate key , operation, value for formating
			for (auto itr_condition = aconditions.begin(); itr_condition != aconditions.end(); itr_condition++)
			{
				check_key(box_id, object_id, *itr_condition);
				check_operation(box_id, object_id, *itr_condition);
				check_value(box_id, object_id, *itr_condition);
			}

			const auto id_condition = find_condition(aconditions, "id");
			const auto author_condition = find_condition(aconditions, "author");
			const auto quantity_condition = find_condition(aconditions, "quantity");

			// check eosio.token contract asset type
			if (assettype_condition_value != TOKEN)
			{
				check(!((name)((string)get<2>(*author_condition)) == EOSIO_TOKEN),
					"Wrong assettype for " + EOSIO_TOKEN.to_string() + " contract for box_id: " + to_string(box_id) + " object_id: " + to_string(object_id) + " . You entered: assettype = " + to_string(assettype_condition_value) + " Must be assettype = 2");
			}

			if (assettype_condition_value == SA_NFT)
			{
				// check will work only if condition no have 'id'. For 'id' it no need 
				if (!hasIDCondition) {
					check(!(count_category != 1), "For assettype = 0 (SA_NFT) must be added category condition. At box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));
				}

				if (id_condition != std::nullopt)
				{
					{
						for (auto itr_nft_tokens = nft_tokens[box_id].begin(); itr_nft_tokens != nft_tokens[box_id].end(); itr_nft_tokens++)
						{
							check(!(atoll(((string)get<2>(*id_condition)).c_str()) == *itr_nft_tokens), "Dublication of id condition for box_id: " + to_string(box_id) + " object_id: " + to_string(object_id) + "condition id = " + (string)get<2>(*id_condition));
						}

						nft_tokens[box_id].emplace_back(atoll(((string)get<2>(*id_condition)).c_str()));
					}
				}
				else if (assettype_condition_value == SA_FT)
				{
					check(!(count_quantity != 1), "For assettype = 1 (SA_FT) must be added quantity condition. box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));
				}
				else if (assettype_condition_value == TOKEN)
				{
					check(!(count_quantity != 1), "For assettype = 2 (TOKEN) must be added quantity condition. box_id: " + to_string(box_id) + " object_id: " + to_string(object_id));
				}

				if (assettype_condition_value == SA_FT || assettype_condition_value == TOKEN)
				{
					check(!(id_condition != std::nullopt), "id condition allowed only for assettype = 0(SA_NFT). At box_id: " + to_string(box_id) + " object_id: " + to_string(object_id) + " .You entered condition id = " + (string)get<2>(*id_condition) + " with assettype = " + to_string(assettype_condition_value));

					const auto author_condition_value = name((string)get<2>(*author_condition));
					const auto quantity_condition_value = asset_from_string((string)get<2>(*quantity_condition));
					// dublicated TOKEN check
					for (auto itr_ft_tokens = ft_tokens[box_id].begin(); itr_ft_tokens != ft_tokens[box_id].end(); itr_ft_tokens++)
					{
						check(!(
							get<0>(*itr_ft_tokens) == author_condition_value &&
							get<1>(*itr_ft_tokens).symbol.code() == quantity_condition_value.symbol.code() &&
							get<2>(*itr_ft_tokens) == assettype_condition_value),
							"Token : " + get<2>(*author_condition) + " " + get<2>(*quantity_condition) + " " + get<2>(*assettype_condition) + " already exist in conditions at box_id: " + to_string(box_id));
					}

					ft_tokens[box_id].emplace_back(make_tuple(author_condition_value, quantity_condition_value, assettype_condition_value));
				}
			}
		}
	}
};