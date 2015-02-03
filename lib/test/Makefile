all:
	@make -C .. -j10  && \
	make -C fa tests && \
	make -C fa_operations tests && \
	make -C fa_rank tests && \
	make -C cfg_rank/c_gram tests

cleanall:
	@make -C fa cleanall && \
	make -C fa_operations cleanall && \
	make -C fa_rank cleanall && \
	make -C cfg_rank/c_gram cleanall
