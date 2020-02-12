#--------------------------------------------------------------------- CREATE ACCEPT PROPOSAL -------------------------------------------------------------------------------

#./wetwax_body.sh barter111111 ACCOUNT1_PROPOSAL ACCOUNT2_ACCEPTING_PROPOSAL AMOUNT_THAT_WANT_ACCOUNT1_FROM_ACCOUNT2 SYMBOL_FOR_AMOUNT AUTHOR_FOR_AMOUNT      AUTHOR_OF_NFT_PROPOSED_FROM_ACCOUNT1 NFT_ID          TEST_NUMBER
#./wetwax_body.sh barter111111 alexnag1tran      alexplayer11                1.1111                                   EOS              eosio.token            alexnag1tran                         100000000004192 1

./wetwax_body.sh barter111111 alexnag1tran alexplayer11 1.1111 EOS eosio.token alexnag1tran 100000000004241 1

./wetwax_body.sh barter111111 alexplayer11 alexnag1tran 1.1111 EOS eosio.token alexnag1tran 100000000004241 1

#--------------------------------------------------------------------- CREATE CANCEL PROPOSAL -------------------------------------------------------------------------------

#./wetwax_body.sh barter111111 alexnag1tran alexplayer11 1.1111 EOS eosio.token alexnag1tran 100000000004192 2

#--------------------------------------------------------------------- CREATE PROPOSAL CREATE OFFER ACCEPT OFFER ------------------------------------------------------------

#./wetwax_body.sh barter111111 alexnag1tran alexplayer11 1.1111 EOS eosio.token alexnag1tran 100000000004192 3
#./wetwax_body.sh barter111111 alexplayer11 alexnag1tran 1.1111 EOS eosio.token alexnag1tran 100000000004192 3

#--------------------------------------------------------------------- DEPOSIT FT  ------------------------------------------------------------

#./wetwax_body.sh barter111111 alexplayer11 0 1.11 TST alexplayer11 0 0 4

#--------------------------------------------------------------------- DEPOSIT WITHDRAW FT ----------------------------------------------------

#./wetwax_body.sh barter111111 alexplayer11 0 1.11 TST alexplayer11 0 0 6

#---------------------------------------------------------------------- TEST_DEPOSIT_WITHDRAW_NFT ---------------------------------------------

#./waxcleos.sh get table simpleassets alexplayer11 sassets -r -l 10

#./wetwax_body.sh barter111111 alexplayer11 0 0 0 0 0 100000000004006 7

#---------------------------------------------------------------------- TEST_REJECT_OFFER ---------------------------------------------

#./wetwax_body.sh barter111111 alexnag1tran alexplayer11 1.1111 EOS eosio.token alexnag1tran 100000000004151 8

#---------------------------------------------------------------------- TEST_MDATA ---------------------------------------------

#./wetwax_body.sh barter111111 alexnag1tran alexplayer11 0 0 0 0 0 9

#---------------------------------------------------------------------- TEST_CREATEWISH_CANCELWISH ---------------------------------------------

#./wetwax_body.sh barter111111 alexnag1tran 0 0 0 0 0 0 10

#---------------------------------------------------------------------- TEST Is integer functionality ---------------------------------------------
#./cleos.sh --verbose push action barter111111 testisint '{"value":"123"}' -p barter111111@active
#./cleos.sh --verbose push action barter111111 testisint '{"value":"qa123"}' -p barter111111@active
#./cleos.sh --verbose push action barter111111 testisint '{"value":"123dd"}' -p barter111111@active
#./cleos.sh --verbose push action barter111111 testisint '{"value":"dddd"}' -p barter111111@active
#---------------------------------------------------------------------- TEST Is integer functionality ---------------------------------------------
