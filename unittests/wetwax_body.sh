echo "Test account where deployed WAXWET: " $1
echo "First account to test transfer: " $2
echo "Second Account to test transfer: " $3

echo "FT amount: " $4
echo "FT symbol: " $5
echo "FT author: " $6
echo "NFT_AUTHOR: " $7
echo "NFT_ID: " $8
echo "TEST Number: " $9
 
BARTER=$1
ACC_1=$2
ACC_2=$3

FT_AMOUNT=$4
FT_SYMBOL=$5
FT_AUTHOR=$6
NFT_AUTHOR=$7
NFT_ID=$8
TEST_ID=$9

WAIT_TIME=2

#---------------------------------------------------------------------------------------------------
# read one record from stproposals table
#---------------------------------------------------------------------------------------------------
function READ_LAST_PROPOSAL
{
	sleep $WAIT_TIME
	size=$(./cleos.sh get table $BARTER $BARTER stproposals -r -l 1 | jq -r '.rows | length')
	proposal_id=$(./cleos.sh get table $BARTER $BARTER stproposals -r -l 1 | jq -r '.rows[].id')
	proposal_owner=$(./cleos.sh get table $BARTER $BARTER stproposals -r -l 1 | jq -r '.rows[].owner')

	if (( size == 1 ))
	  then
		result_proposal_id=$proposal_id
	  else
		echo "Error. stproposals table is empty"
		./cleos.sh get table $BARTER $BARTER stproposals 
		exit 1
	fi
}

#---------------------------------------------------------------------------------------------------
# read one record from sconditions table
#---------------------------------------------------------------------------------------------------
function READ_LAST_SCONDITIONS
{
	sleep $WAIT_TIME
	size=$(./cleos.sh get table $BARTER $BARTER sconditions -r -l 1 | jq -r '.rows | length')
	condition_id=$(./cleos.sh get table $BARTER $BARTER sconditions -r -l 1 | jq -r '.rows[].id')
	cproposal_id=$(./cleos.sh get table $BARTER $BARTER sconditions -r -l 1 | jq -r '.rows[].proposalid')
	condition_owner=$(./cleos.sh get table $BARTER $BARTER sconditions -r -l 1 | jq -r '.rows[].owner')
	condition_boxid=$(./cleos.sh get table $BARTER $BARTER sconditions -r -l 1 | jq -r '.rows[].boxid')
	condition_objectid=$(./cleos.sh get table $BARTER $BARTER sconditions -r -l 1 | jq -r '.rows[].objectid')

	if (( size == 1 ))
	  then
		result_proposal_id=$cproposal_id
		result_condition_id=$condition_id
		result_condition_owner=$condition_owner
		result_condition_boxid=$condition_boxid
		result_condition_objectid=$condition_objectid
	  else
		echo "Error. sconditions table is empty"
		./cleos.sh get table $BARTER $BARTER sconditions 
		exit 1
	fi
}

#ACC_1

function CREATE_PROPOSAL
{
	echo "---------------------------------------------- createprop -----------------------------------------------------"
	sleep $WAIT_TIME

	./cleos.sh  --verbose push action $BARTER createprop '{ "owner":"'${ACC_1}'", "nfts":['${NFT_ID}'], "fts":[], "fees":{"exchange": "name","exchange_fee": 10,"affiliate_id": 20,"affiliate_fee": 30}  ,"conditions": [[1, 1, "author","=","'${FT_AUTHOR}'"], [1, 1, "assettype","=","1"], [1, 1, "quantity","=","'${FT_AMOUNT}' '${FT_SYMBOL}'"]], "topropid":0, "account_to":"", "datestart":0, "dateexpire":0, "auto_accept":true, "memo":"" }' -p $ACC_1@active
}

function REJECT_OFFER
{
	echo "---------------------------------------------- rejectoffer -----------------------------------------------------"
	sleep $WAIT_TIME

	./cleos.sh  --verbose push action $BARTER rejectoffer '{ "owner":"'${ACC_1}'", "offer_id":'${result_offer_id}' }' -p $ACC_1@active
}

#ACC_2
function ACCEPT_PROPOSAL_ACC2
{
	echo "---------------------------------------------- accept proposal -----------------------------------------------------"

	sleep $WAIT_TIME
	./cleos.sh --verbose push action $BARTER acceptprop '{ "owner":"'${ACC_2}'", "proposal_id": '${result_proposal_id}', "box_id":1}' -p $ACC_2@active
}

function CREATE_OFFER
{
	echo "---------------------------------------------- create offer -----------------------------------------------------"
	sleep $WAIT_TIME

	./cleos.sh --verbose push action $BARTER createprop '{ "owner":"'${ACC_2}'", "nfts":[], "fts":[["'${FT_AUTHOR}'", "2.2222 '${FT_SYMBOL}'", 1]], "fees":{"exchange": "name","exchange_fee": 10,"affiliate_id": 20,"affiliate_fee": 30}  ,"conditions": [[1,1,"author","=","'${NFT_AUTHOR}'"], [1, 1, "assettype","=","1"] , [1,1,"id","=","'${NFT_ID}'"]], "topropid":'${result_proposal_id}', "account_to":"", "datestart":0, "dateexpire":0, "auto_accept":true , "memo":"" }' -p $ACC_2@active
}

#ACC_1

function ACCEPT_OFFER
{
	echo "---------------------------------------------- accept offer -----------------------------------------------------"

	sleep $WAIT_TIME
	./cleos.sh --verbose push action $BARTER acceptprop '{ "owner":"'${ACC_1}'", "proposal_id": '${result_offer_id}', "box_id":1}' -p $ACC_1@active
}


function GET_BALANCES
{
	GET_BALANCE1
	GET_BALANCE2
}

function GET_BALANCE1
{
	#echo "---------------------------------------------- getbalance Account 1 -----------------------------------------------------"

 	BALANCE1=$(./cleos.sh --verbose push action $BARTER getbalance '{ "owner":"'${ACC_1}'", "author":"'${FT_AUTHOR}'" "sym":"'${FT_SYMBOL}'" }' -p $ACC_1@active)
}

function GET_BALANCE2
{
	#echo "---------------------------------------------- getbalance Account 2 -----------------------------------------------------"
	BALANCE2=$(./cleos.sh --verbose push action $BARTER getbalance '{ "owner":"'${ACC_2}'", "author":"'${FT_AUTHOR}'" "sym":"'${FT_SYMBOL}'" }' -p $ACC_2@active)
}

function TEST_ACCEPT_PROPOSAL
{
	echo "---------------------------------------------- START TEST_ACCEPT_PROPOSAL -----------------------------------------------------"

	CREATE_PROPOSAL
	
	READ_LAST_PROPOSAL

	echo "-proposal_id-: " ${result_proposal_id}
	echo "proposal_owner: " ${proposal_owner}
	sleep $WAIT_TIME

	READ_LAST_SCONDITIONS

	echo "-result_condition_id-: " ${result_condition_id}
	echo "result_proposal_id: " ${result_proposal_id}
	echo "result_condition_owner: " ${result_condition_owner}
	echo "result_condition_boxid: " ${result_condition_boxid}

	echo "result_condition_objectid: " ${result_condition_objectid}

	GET_BALANCES

	ACCEPT_PROPOSAL_ACC2

	echo "---------------------------------------------- before -----------------------------------------------------"
	PRINT_BALANCES

	GET_BALANCES

	echo "---------------------------------------------- after -----------------------------------------------------"

	PRINT_BALANCES

	echo "---------------------------------------------- END TEST_ACCEPT_PROPOSAL -----------------------------------------------------"
}

function PRINT_BALANCES
{
	PRINT_BALANCE1
	PRINT_BALANCE2
}

function PRINT_BALANCE1
{
	echo "Balance for " ${ACC_1} " : " ${BALANCE1}
}

function PRINT_BALANCE2
{
	echo "Balance for " ${ACC_2} " : " ${BALANCE2}
}


function CANCEL_PROPOSAL
{
	echo "---------------------------------------------- cancel proposal -----------------------------------------------------"
	sleep $WAIT_TIME

	./cleos.sh push action $BARTER cancelprop '{ "owner":"'${ACC_1}'", "proposal_id":'${result_proposal_id}'}'  -p $ACC_1@active
}

function TEST_CREATE_CANCEL_PROPOSAL
{
	echo "---------------------------------------------- START TEST_CREATE_CANCEL_PROPOSAL -----------------------------------------------------"

	CREATE_PROPOSAL
	
	READ_LAST_PROPOSAL

	echo "-proposal_id-: " ${result_proposal_id}
	echo "proposal_owner: " ${proposal_owner}
	sleep $WAIT_TIME

	READ_LAST_SCONDITIONS

	echo "-result_condition_id-: " ${result_condition_id}
	echo "result_proposal_id: " ${result_proposal_id}
	echo "result_condition_owner: " ${result_condition_owner}
	echo "result_condition_boxid: " ${result_condition_boxid}

	echo "result_condition_objectid: " ${result_condition_objectid}

	DISPLAY_TABLES_CONDITIONS_PROPOSALS

	CANCEL_PROPOSAL

	DISPLAY_TABLES_CONDITIONS_PROPOSALS

	echo "---------------------------------------------- END TEST_CREATE_CANCEL_PROPOSAL -----------------------------------------------------"
}

function DISPLAY_TABLES_CONDITIONS_PROPOSALS
{
	echo "---------------------------------------------- Last row of TABLE stproposals -----------------------------------------------------"
	./cleos.sh get table $BARTER $BARTER stproposals -r -l 1
	echo "---------------------------------------------- Last row of TABLE sconditions -----------------------------------------------------"
	./cleos.sh get table $BARTER $BARTER sconditions -r -l 1
}

function TEST_CREATE_PROPOSAL_CREATE_OFFER_ACCEPT_OFFER
{
	echo "---------------------------------------------- START TEST_CREATE_PROPOSAL_CREATE_OFFER_ACCEPT_OFFER ---------------------------------------------------"

	CREATE_PROPOSAL
	
	READ_LAST_PROPOSAL

	echo "-proposal_id-: " ${result_proposal_id}
	echo "proposal_owner: " ${proposal_owner}
	sleep $WAIT_TIME

	READ_LAST_SCONDITIONS

	echo "-result_condition_id-: " ${result_condition_id}
	echo "result_proposal_id: " ${result_proposal_id}
	echo "result_condition_owner: " ${result_condition_owner}
	echo "result_condition_boxid: " ${result_condition_boxid}

	echo "result_condition_objectid: " ${result_condition_objectid}

	CREATE_OFFER
	READ_LAST_PROPOSAL
	result_offer_id=${result_proposal_id}

	GET_BALANCES

	ACCEPT_OFFER

	echo "---------------------------------------------- before -----------------------------------------------------"
	PRINT_BALANCES

	GET_BALANCES

	echo "---------------------------------------------- after -----------------------------------------------------"

	PRINT_BALANCES

	echo "---------------------------------------------- END TEST_CREATE_PROPOSAL_CREATE_OFFER_ACCEPT_OFFER -----------------------------------------------------"
}

function TEST_DEPOSIT_FT
{
	echo "---------------------------------------------- TEST_DEPOSIT_FT -----------------------------------------------------"

	GET_BALANCE1

	CREATEF_GOLD_FOR_ACC1
	TRANSFERF_GOLD_FOR_ACC1

	#./cleos.sh push action simpleassets transferf '{"from":"'${ACC_1}'", "to":"'${BARTER}'", "author":"'${FT_AUTHOR}'", "quantity":"'${FT_AMOUNT}' '${FT_SYMBOL}'", "memo":"memo_here"}' -p $ACC_1@active

	PRINT_BALANCE1
	GET_BALANCE1
	PRINT_BALANCE1
	echo "---------------------------------------------- END TEST_DEPOSIT_FT --------------------------------------------------"
}

function TEST_DEPOSIT_NFT
{
	echo "---------------------------------------------- TEST_DEPOSIT_NFT -----------------------------------------------------"

	./cleos.sh push action simpleassets transfer '{ "from":"'${ACC_1}'", "to":"'${BARTER}'" , "assetids":['${NFT_ID}'], "memo":"" }' -p $ACC_1@active

	./cleos.sh get table $BARTER $BARTER sinventory -r -l 1

	echo "---------------------------------------------- END TEST_DEPOSIT_NFT --------------------------------------------------"
}

function TEST_DEPOSIT_WITHDRAW_FT
{
	echo "---------------------------------------------- TEST_DEPOSIT_WITHDRAW_FT -----------------------------------------------------"

	GET_BALANCE1

	./cleos.sh push action simpleassets transferf '{"from":"'${ACC_1}'", "to":"'${BARTER}'", "author":"'${FT_AUTHOR}'", "quantity":"'${FT_AMOUNT}' '${FT_SYMBOL}'", "memo":"memo_here"}' -p $ACC_1@active

	PRINT_BALANCE1

	GET_BALANCE1

	./cleos.sh --verbose push action $BARTER withdraw '{ "owner":"'${ACC_1}'", "nfts":[] , "fts":[["'${FT_AUTHOR}'", "'${FT_AMOUNT}' '${FT_SYMBOL}'", 1]] }' -p $ACC_1@active

	PRINT_BALANCE1

	GET_BALANCE1

	PRINT_BALANCE1

	echo "---------------------------------------------- END TEST_DEPOSIT_WITHDRAW_FT --------------------------------------------------"
}

function TEST_DEPOSIT_WITHDRAW_NFT
{
	echo "---------------------------------------------- TEST_DEPOSIT_WITHDRAW_NFT -----------------------------------------------------"

	./cleos.sh push action simpleassets transfer '{ "from":"'${ACC_1}'", "to":"'${BARTER}'" , "assetids":['${NFT_ID}'], "memo":"" }' -p $ACC_1@active

	./cleos.sh get table $BARTER $BARTER sinventory -r -l 1

	./cleos.sh --verbose push action $BARTER withdraw '{ "owner":"'${ACC_1}'", "nfts":['${NFT_ID}'] , "fts":[] }' -p $ACC_1@active

	./cleos.sh get table $BARTER $BARTER sinventory -r -l 1

	echo "---------------------------------------------- END TEST_DEPOSIT_WITHDRAW_NFT --------------------------------------------------"
}

function TEST_REJECT_OFFER
{
	echo "---------------------------------------------- START TEST_REJECT_OFFER ---------------------------------------------------"

	CREATE_PROPOSAL
	
	READ_LAST_PROPOSAL
	first_proposal_id=${result_proposal_id}

	echo "-proposal_id-: " ${result_proposal_id}
	echo "proposal_owner: " ${proposal_owner}
	sleep $WAIT_TIME

	READ_LAST_SCONDITIONS

	echo "-result_condition_id-: " ${result_condition_id}
	echo "result_proposal_id: " ${result_proposal_id}
	echo "result_condition_owner: " ${result_condition_owner}
	echo "result_condition_boxid: " ${result_condition_boxid}

	echo "result_condition_objectid: " ${result_condition_objectid}

	CREATE_OFFER
	READ_LAST_PROPOSAL
	result_offer_id=${result_proposal_id}
	./cleos.sh get table $BARTER $BARTER stproposals -r -l 1
	REJECT_OFFER
	./cleos.sh get table $BARTER $BARTER stproposals -r -l 1

	result_proposal_id=${first_proposal_id}
	
	CANCEL_PROPOSAL
	./cleos.sh get table $BARTER $BARTER stproposals -r -l 1

	echo "---------------------------------------------- END TEST_REJECT_OFFER ---------------------------------------------------"
}

#---------------------------------------------------------------------------------------------------
# read last record from sassets table
#---------------------------------------------------------------------------------------------------
function READ_LAST_ASSET
{
	sleep $WAIT_TIME
	asset_ids=$(./cleos.sh get table $ACC $SCOPE sassets -r -l 1 | jq -r '.rows[].id')
	asset_author=$(./cleos.sh get table $ACC $SCOPE sassets | jq -r '.rows[].author')

	result_asset_id=$asset_ids
}

function CREATEF_GOLD_FOR_ACC1
{
	./cleos.sh push action simpleassets createf '{"author":"'${ACC_1}'", "maximum_supply":"1200000.00 GOLD", "authorctrl":0, "data":"dfd"}' -p $ACC_1@active
	./cleos.sh push action simpleassets issuef  '["'${ACC_1}'", "'${ACC_1}'", "10.00 GOLD", "test"]' -p $ACC_1@active
}

function TRANSFERF_GOLD_FOR_ACC1
{
	sleep $WAIT_TIME
	./cleos.sh push action simpleassets transferf '{"from":"'${ACC_1}'", "to":"'${BARTER}'", "author":"'${ACC_1}'", "quantity":"1.00 GOLD", "memo":"memo_here"}' -p $ACC_1@active
}

function CREATE_NFT_FOR_ACC2
{
	./cleos.sh push action simpleassets create '{ "author":"'${ACC_2}'", "category":"tst", "owner":"'${ACC_2}'" ,"idata":"idata","mdata":"{\"category\":{\"category\":{\"category\":333,\"img\":\"c\",\"prize\":0,\"p\":15},\"img\":\"c\",\"prize\":0,\"p\":15},\"img\":\"c\",\"prize\":0,\"p\":15}" , "requireclaim":0 }' -p $ACC_2@active
	sleep $WAIT_TIME

	asset_id=$(./cleos.sh get table simpleassets $ACC_2 sassets -r -l 1 | jq -r '.rows[].id')
	echo "asset_id: " ${asset_id}
}

function TRANSFER_NFT_FOR_ACC2
{
	./cleos.sh push action simpleassets transfer '{ "from":"'${ACC_2}'", "to":"'${BARTER}'" , "assetids":['${asset_id}'], "memo":"" }' -p $ACC_2@active
}

function TEST_MDATA
{
	# before_asset_id=$(./cleos.sh get table simpleassets $ACC_2 sassets -r -l 1 | jq -r '.rows[].id')
	# echo "before_asset_id: " ${before_asset_id}

	CREATE_NFT_FOR_ACC2
	TRANSFER_NFT_FOR_ACC2

	./cleos.sh get table $BARTER $BARTER sinventory -r -l 1

	CREATEF_GOLD_FOR_ACC1
	TRANSFERF_GOLD_FOR_ACC1


	CREATE_PROPOSAL_MDATA_ACC1

	READ_LAST_PROPOSAL

	ACCEPT_PROPOSAL_ACC2
}

function CREATE_PROPOSAL_MDATA_ACC1
{
	echo "---------------------------------------------- createprop -----------------------------------------------------"
	sleep $WAIT_TIME

	./cleos.sh --verbose push action $BARTER createprop '{ "owner":"'${ACC_1}'", "nfts":[], "fts":[["'${ACC_1}'", "1.00 GOLD", 1]], "fees":{"exchange": "name","exchange_fee": 10,"affiliate_id": 20,"affiliate_fee": 30}  ,"conditions": [[1 , 1, "author","=","'${ACC_2}'"], [1 , 1, "assettype","=","1"], [1, 1, "mdata.category.category.category","=","333"]], "topropid":0, "account_to":"", "datestart":0, "dateexpire":0, "auto_accept":true, "memo":"" }' -p $ACC_1@active
}


function TEST_CREATEWISH_CANCELWISH
{
	echo "---------------------------------------------- START TEST_CREATEWISH_CANCELWISH ---------------------------------------------------"

	before_wish_id=$(./cleos.sh get table $BARTER $BARTER swish -r -l 1 | jq -r '.rows[].id')

	sleep $WAIT_TIME

	./cleos.sh push action $BARTER createwish '{ "owner":"'${ACC_1}'", "conditions": [["author","=","'${ACC_1}'"], ["assettype","=","1"] ,["quantity","=","1.1111 GOLD"]] }' -p $ACC_1@active

	sleep $WAIT_TIME
	created_wish_id=$(./cleos.sh get table $BARTER $BARTER swish -r -l 1 | jq -r '.rows[].id')
	echo "after_create_wish_id: " ${after_create_wish_id}

	if (( before_wish_id == created_wish_id ))
	  then
		echo "before_wish_id: " ${before_wish_id}
		echo "Error. Wish was not added to table"
		./cleos.sh get table $BARTER $BARTER swish -r -l 20 
		exit 1
	  else
		echo "Success. Wish was added to swish table"
	fi

	./cleos.sh push action $BARTER cancelwish '{ "owner":"'${ACC_1}'", "wish_id":'${created_wish_id}' }' -p $ACC_1@active

	sleep $WAIT_TIME
	after_cancel_wish_id=$(./cleos.sh get table $BARTER $BARTER swish -r -l 1 | jq -r '.rows[].id')

	if (( after_cancel_wish_id == created_wish_id ))
	  then
		echo "Error. Wish was not removed from swish table"
		echo "after_cancel_wish_id: " ${after_cancel_wish_id}
		echo "after_create_wish_id: " ${after_create_wish_id}
		./cleos.sh get table $BARTER $BARTER swish -r -l 20 
		exit 1
	  else
		echo "Success. Wish was removed from swish table"
	fi

	echo "---------------------------------------------- END TEST_CREATEWISH_CANCELWISH ---------------------------------------------------"
}

if (( TEST_ID == 1 ))
then
	TEST_ACCEPT_PROPOSAL
fi

if (( TEST_ID == 2 ))
then
	TEST_CREATE_CANCEL_PROPOSAL
fi

if (( TEST_ID == 3 ))
then
	TEST_CREATE_PROPOSAL_CREATE_OFFER_ACCEPT_OFFER
fi

if (( TEST_ID == 4 ))
then
	TEST_DEPOSIT_FT
fi

if (( TEST_ID == 5 ))
then
	TEST_DEPOSIT_NFT
fi

if (( TEST_ID == 6 ))
then
	TEST_DEPOSIT_WITHDRAW_FT
fi

if (( TEST_ID == 7 ))
then
	TEST_DEPOSIT_WITHDRAW_NFT
fi

if (( TEST_ID == 8 ))
then
	TEST_REJECT_OFFER
fi

if (( TEST_ID == 9 ))
then
	TEST_MDATA
fi

if (( TEST_ID == 10 ))
then
	TEST_CREATEWISH_CANCELWISH
fi
