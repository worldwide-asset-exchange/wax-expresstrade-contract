--- Wax Express Trade Project ---

 - How to Build -
   - cd to 'build' directory
   - run the command 'cmake ..'
   - run the command 'make'

 - After build -
   - The built smart contract is under the 'Barter' directory in the 'build' directory
   - You can then do a 'set contract' action with 'cleos' and point in to the './build/Barter' directory

 - Additions to CMake should be done to the CMakeLists.txt in the './src' directory and not in the top level CMakeLists.txt

----------------- WAX WET 2.0 migration important steps -----------------

Before deploying need to do migration

1) deploy wasm from folder Migration1
build\Migration\Migration1
and call action migrate
./cleos.sh  --verbose push action waxexpresstrade migrate '[]' -p alexplayer11

2) deploy wasm from folder Migration2
build\Migration\Migration2
and call action migrateback
./cleos.sh  --verbose push action waxexpresstrade migrateback '[]' -p alexplayer11

3) deploy wasm from folder build
build\
continue work with this wasm 

----------------- WAX WET 2.0 Setup -----------------

Important steps before deploying:
1) Create eosio account for receiceving community fee.
2) Change in wax-expresstrade-contract\include\common.h file variable with created name of community fee account:
const name COMMUNITY_FEE_ACCOUNT   = "communityfee"_n;
3) rebuild and deploy wax wet express wasm

If you want to test receiving author`s fee.
1) Create simpleassets record for author of NFT.
./cleos.sh push action simpleassets regauthor '["nft_author_here", "{\"defaultfee\":333}", "", "" ]' -p nft_author_here@active
author`s fee is in dappinfo parameter in json format
Example: "{\"defaultfee\":333}".
Author`s fee must be between 0 and 500.

2) For changing author`s fee use authorupdate action:
./cleos.sh push action simpleassets authorupdate '[ "nft_author_here", "{\"defaultfee\":333}", "", "" ]' -p nft_author_here@active

------------------------------------------------------
