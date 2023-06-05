_sp_load_block:
	sta sp_dispatch + 1
sp_dispatch:
	jsr $c700
	.byte $01
	.word sp_params
	rts

sp_params:
	.byte $03
	.byte $01
_sp_load_block_dest_addr:
	.byte $00, $00
_sp_load_block_source_lomed:
	.byte $00, $00
_sp_load_block_source_high:
	.byte $00
	rts

.export	_sp_load_block
.export _sp_load_block_dest_addr
.export _sp_load_block_source_lomed
.export _sp_load_block_source_high
