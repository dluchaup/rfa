all:
	@make -C .. -j10  && \
	make -C fa tests && \
	make -C fa_rank tests

cleanall:
	@make -C fa cleanall && \
	make -C fa_rank cleanall
