#include<stdio.h>
#include<stdint.h>
#include<string.h>
#include<6502.h>

#define ZERO_STRUCT(X) bzero ( &X , sizeof ( X ) )

extern uint8_t dirbuf[512];
extern uint8_t databuf[512]; // for data block
extern uint8_t indexbuf[512]; // for index block
extern uint8_t masterbuf[512]; // for master block

#define MAX_DIR_ENTRIES = 0xd;

static uint8_t dispatch_offset;
uint8_t dirbuf[512];
uint8_t databuf[512];
uint8_t indexbuf[512];
uint8_t masterbuf[512];

uint8_t* sdhr_command_p = (uint8_t*)0xc0b0;
uint8_t* sdhr_data_p = (uint8_t*)0xc0b1;
uint8_t* key_p = (uint8_t*)0xc000;
uint8_t* key_strobe_p = (uint8_t*)0xc010;

uint8_t avatar_tile_data[] = { 1, 28  }; // location of avatar tile in tilesets

// Moons tileset definition
uint8_t ts_moons[] = {
  0x0, 0x0, 0x0, 0x0, 
  0x1, 0x0, 0x0, 0x0, 
  0x2, 0x0, 0x0, 0x0, 
  0x3, 0x0, 0x0, 0x0, 
  0x4, 0x0, 0x0, 0x0, 
  0x5, 0x0, 0x0, 0x0, 
  0x6, 0x0, 0x0, 0x0, 
  0x7, 0x0, 0x0, 0x0};
uint8_t moons_tile_data[] = { 2, 1, 2, 5 }; // 2 moons

extern uint8_t fastcall sp_load_block(uint8_t offset);
extern uint8_t* sp_load_block_dest_addr;
extern uint16_t sp_load_block_source_lomed;
extern uint8_t sp_load_block_source_high;

struct file_entry {
  uint8_t storage_type_name_length;
  char filename[15];
  uint8_t file_type;
  uint16_t block;
  uint16_t blocks_used;
  uint8_t eof[3];
  uint8_t create_date_time[4];
  uint8_t version;
  uint8_t min_version;
  uint8_t access;
  uint16_t aux_type;
  uint8_t last_mod[4];
  uint8_t* header_pointer;
};

struct dir_block {
  uint16_t prev_next[2];
  struct file_entry entries[13];
  uint8_t padding;
};

/* struct index_block { */
/*   uint8_t low_blocks[256]; */
/*   uint8_t high_blocks[256]; */
/* }; */

struct upload_data_cmd_t {
  uint16_t cmd_len;
  uint8_t cmd_id;
  uint16_t dest_block;
  uint16_t source_addr;
} upload_data_cmd;

struct define_image_asset_cmd_t {
  uint16_t cmd_len;
  uint8_t cmd_id;
  uint8_t asset_id;
  uint16_t block_count;
} define_image_asset_cmd;

struct define_tileset_cmd_t {
  uint16_t cmd_len;
  uint8_t cmd_id;
  uint8_t tileset_index;
  uint8_t asset_index;
  uint8_t num_entries;
  uint16_t xdim;
  uint16_t ydim;
  uint16_t block_count;
} define_tileset_cmd;

struct define_tileset_set_immediate_cmd_t {
  uint16_t cmd_len;
  uint8_t cmd_id;
  uint8_t tileset_index;
  uint8_t asset_index;
  uint8_t num_entries;
  uint8_t xdim;
  uint8_t ydim;
} define_tileset_set_immediate_cmd;

struct define_window_cmd_t {
  uint16_t cmd_len;
  uint8_t cmd_id;
  uint8_t window_index;
  uint16_t screen_xcount;
  uint16_t screen_ycount;
  uint16_t tile_xdim;
  uint16_t tile_ydim;
  uint16_t tile_xcount;
  uint16_t tile_ycount;
} define_window_cmd;

struct update_window_set_upload_cmd_t {
  uint16_t cmd_len;
  uint8_t cmd_id;
  uint8_t window_index;
  uint16_t block_count;
} update_window_set_upload_cmd;

struct update_window_set_immediate_cmd_t {
  uint16_t cmd_len;
  uint8_t cmd_id;
  uint8_t window_index;
  uint16_t data_len;
} update_window_set_immediate_cmd;

struct update_window_enable_cmd_t {
  uint16_t cmd_len;
  uint8_t cmd_id;
  uint8_t window_index;
  uint8_t enabled;
  uint32_t anim_ms_frame;
} update_window_enable_cmd;

struct update_window_view_cmd_t {
  uint16_t cmd_len;
  uint8_t cmd_id;
  uint8_t window_index;
  int32_t screen_xbegin;
  int32_t screen_ybegin;
} update_window_view_cmd;

struct update_window_position_cmd_t {
  uint16_t cmd_len;
  uint8_t cmd_id;
  uint8_t window_index;
  int32_t screen_xbegin;
  int32_t screen_ybegin;
} update_window_position_cmd;

struct update_window_size_cmd_t {
  uint16_t cmd_len;
  uint8_t cmd_id;
  uint8_t window_index;
  int32_t screen_xcount;
  int32_t screen_ycount;
} update_window_size_cmd;

struct change_resolution_cmd_t {
  uint16_t cmd_len;
  uint8_t cmd_id;
  uint32_t width;
  uint32_t height;
} change_resolution_cmd;

struct update_window_display_image_cmd_t {
  uint16_t cmd_len;
  uint8_t cmd_id;
  uint8_t window_index;
  uint8_t asset_index;
  uint8_t pixel_size;
} update_window_display_image_cmd;

uint16_t upload_file(const char* fn);
void process_seedling(uint16_t block, uint16_t* current_block);
void process_sapling(uint16_t block, uint16_t block_count, uint16_t* current_block);
void process_tree(uint16_t block, uint16_t block_count, uint16_t* current_block);
void queue_command(void* cmd);
void queue_data(uint16_t data_len, void* data);
void disable_sdhr(void);
void enable_sdhr(void);
void process_commands(void);
void reset_sdhr(void);

void create_image_asset(uint8_t asset_index, uint16_t block_count);
void define_tileset(uint8_t tileset_index, uint8_t asset_index, 
		    uint8_t num_entries, uint16_t xdim, uint16_t ydim,
		    uint16_t block_count);
void define_tileset_set_immediate(uint8_t tileset_index, uint8_t asset_index, 
		    uint8_t num_entries, uint16_t xdim, uint16_t ydim, uint8_t* data);
void define_window(uint8_t window_index, 
		   uint16_t screen_xcount, uint16_t screen_ycount,
		   uint16_t tile_xdim, uint16_t tile_ydim,
		   uint16_t tile_xcount, uint16_t tile_ycount);
void update_window_set_upload(uint8_t window_index, uint16_t block_count);
void update_window_set_immediate(uint8_t window_index, 
				 uint16_t data_len, uint8_t* data);
void update_window_enable(uint8_t window_index, uint8_t enabled, uint32_t anim_ms_frame);
void update_window_position(uint8_t window_index, 
			    int32_t screen_xbegin, int32_t screen_ybegin);
void update_window_view(uint8_t window_index, 
			int32_t screen_xbegin, int32_t screen_ybegin);
void update_window_size(uint8_t window_index, 
			int32_t screen_xcount, int32_t screen_ycount);
void change_resolution(uint32_t width, uint32_t height);
void update_window_display_image(uint8_t window_index, uint8_t asset_index, uint8_t pixel_size);
void init_commands(void);

uint16_t main()
{
  uint8_t key;
  uint8_t strobe;
  uint16_t block_count;
  int32_t xpos = 1072;
  int32_t ypos = 1584;
  uint8_t i = 0;
  uint8_t scroll_speed = 1;
  
  uint32_t __width = 640;
  uint32_t __height = 360;
  uint8_t is1080p = 0;

  init_commands();
  enable_sdhr();

  change_resolution(__width, __height);
  process_commands();

  // Startup splash screens
  block_count = upload_file("U5STARTUP1.PNG");
  create_image_asset(11, block_count);
  process_commands();
  define_window(11,640,360,640,360,1,1);
  process_commands();
  update_window_display_image(11, 11, 1);
  update_window_enable(11, 1, 0);
  process_commands();

  block_count = upload_file("U5STARTUP2.PNG");
  create_image_asset(10, block_count);
  process_commands();
  define_window(10,640,360,640,360,1,1);
  process_commands();
  update_window_display_image(10,10, 16);
  process_commands();

  while (1) {
    key = *key_p;
    if (key & 0x80) {
      strobe = *key_strobe_p;
      break;
    }
  }

  update_window_enable(10, 1, 0);
  process_commands();

  block_count = upload_file("U5TILES1.PNG");
  create_image_asset(0, block_count);
  process_commands();
  block_count = upload_file("U5TILES2.PNG");
  create_image_asset(1, block_count);
  process_commands();
  block_count = upload_file("U5TILES3.PNG");
  create_image_asset(2, block_count);
  process_commands();
  block_count = upload_file("U5TILES4.PNG");
  create_image_asset(3, block_count);
  process_commands();
  
  block_count = upload_file("U5TILEIDX0");
  define_tileset(0,0,0,16,16,block_count);
  process_commands();
  
  define_window(0,__width,__height,16,16,256,256);
  
  block_count = upload_file("BRITANNIA.GZ");
  update_window_set_upload(0,block_count);
  process_commands();
  update_window_view(0,xpos,ypos); // center near castle
  process_commands();

  block_count = upload_file("U5TILEIDX1");
  define_tileset(1,0,0,16,16,block_count);
  process_commands();
  define_window(1,16,16,16,16,1,1);
  update_window_set_immediate(1,2,avatar_tile_data);
  update_window_position(1,(__width/2 - 16), (__height/2 - 16));
  process_commands();

  // insert 2 moons at the top of the screen
  block_count = upload_file("MOONS.PNG");
  create_image_asset(4, block_count);
  process_commands();
  block_count = upload_file("MOONSIDX0");
  define_tileset(2,4,8,64,64,block_count);
  process_commands();
  //define_tileset_set_immediate(2,1,8,64,64,ts_moons);
  define_window(2,128,64,64,64,2,1);
  update_window_set_immediate(2,4,moons_tile_data);
  update_window_position(2,248,8);
  process_commands();

  while (1) {
    key = *key_p;
    if (key & 0x80) {
      strobe = *key_strobe_p;
      break;
    }
  }
  
  update_window_enable(11,0, 0);  // disable startup screen
  update_window_enable(10,0, 0);
  process_commands();
  update_window_enable(0,1, 500);
  update_window_enable(1,1, 250);
  update_window_enable(2,1, 0);
  process_commands();
  
  while (1) {
    key = *key_p;
    if (key & 0x80) {
      strobe = *key_strobe_p;
      key &= 0x7f;
      switch (key) {
      case 3: // ctrl-c
	disable_sdhr();
	return 0;
      case 8: // left arrow
	for (i = 0; i < 16; i += scroll_speed) {
	  xpos -= scroll_speed ;
	  if (xpos < 0) {
//	    xpos += 65536;
	  }
	  update_window_view(0,xpos,ypos);
	  process_commands();
	}
	break;
      case 10: // down arrow
	for (i = 0; i < 16; i += scroll_speed) {
	  ypos += scroll_speed;
	  if (ypos >= 65536) {
//	    ypos -= 65536;
	  }
	  update_window_view(0,xpos,ypos);
	  process_commands();
	}
	break;
      case 11: // up arrow
	for (i = 0; i < 16; i += scroll_speed) {
	  ypos -= scroll_speed;
	  if (ypos < 0) {
//	    ypos += 65536;
	  }
	  update_window_view(0,xpos,ypos);
	  process_commands();
	}
	break;
      case 21: // right arrow
	for (i = 0; i < 16; i += scroll_speed) {
	  xpos += scroll_speed;
	  if (xpos >= 65536) {
//	    xpos -= 65536;
	  }
	  update_window_view(0,xpos,ypos);
	  process_commands();
	}
	break;
      case 43: // +
	  update_window_enable(0,1,100);
	  update_window_enable(1,1,20);
	  process_commands();
	break;
      case 45: // -
	  update_window_enable(0,1,500);
	  update_window_enable(1,1,250);
	  process_commands();
	break;
      case 70: // F, for faster scroll
	scroll_speed = 16;
	break;
      case 83: // S, for slower scroll
	scroll_speed = 1;
	break;
      case 82: // R. for resolution
	if (is1080p == 1)
	  break;
      	{
	uint32_t w = 1920;
	uint32_t h = 1080;
	change_resolution(w,h);
	is1080p = 1;
	// window 0 is the overland
	// resize it to the full screen and then
	// change its origin so it stays centered
	update_window_size(0,w,h);
	xpos = xpos - (w-__width)/2;
	ypos = ypos - (h-__height)/2;
        update_window_view(0,xpos,ypos); // center near castle
  	update_window_position(1,(w/2 - 16), (h/2 - 16));
  	update_window_position(2,(w - 128)/2,8);
	process_commands();
	break;
      	}
      case 84: // T.: Revert resolution 
	if (is1080p == 0)
	  break;
      	{
	uint32_t w = 1920;
	uint32_t h = 1080;
	change_resolution(__width,__height);
	is1080p = 0;
	update_window_size(0,__width,__height);
	xpos = xpos + (w-__width)/2;
	ypos = ypos + (h-__height)/2;
        update_window_view(0,xpos,ypos); // center near castle
  	update_window_position(1,(__width/2 - 16), (__height/2 - 16));
  	update_window_position(2,(__width - 128)/2,8);
	process_commands();
	break;
      	}
      }
      //printf("%u\n",key);
    }
  }
  return 0;
}

void init_commands() {
  // determine low byte of smartport dispatch address
  __asm__("lda $C7FF");
  __asm__("clc");
  __asm__("adc #$03");
  __asm__("sta %v", dispatch_offset);
  // initialize constant params of commands
  ZERO_STRUCT(upload_data_cmd);
  upload_data_cmd.cmd_len = sizeof(upload_data_cmd);
  upload_data_cmd.cmd_id = 1;
  /* printf("%p\n",databuf); */
  /* BRK(); */

  ZERO_STRUCT(define_image_asset_cmd);
  define_image_asset_cmd.cmd_len = sizeof(define_image_asset_cmd);
  define_image_asset_cmd.cmd_id = 2;

  ZERO_STRUCT(define_tileset_cmd);
  define_tileset_cmd.cmd_len = sizeof(define_tileset_cmd);
  define_tileset_cmd.cmd_id = 4;

  ZERO_STRUCT(define_tileset_set_immediate_cmd);
  define_tileset_set_immediate_cmd.cmd_len = 
    sizeof(define_tileset_set_immediate_cmd);
  define_tileset_set_immediate_cmd.cmd_id = 5;

  ZERO_STRUCT(define_window_cmd);
  define_window_cmd.cmd_len = sizeof(define_window_cmd);
  define_window_cmd.cmd_id = 6;

  ZERO_STRUCT(update_window_set_upload_cmd);
  update_window_set_upload_cmd.cmd_len = sizeof(update_window_set_upload_cmd);
  update_window_set_upload_cmd.cmd_id = 16;

  ZERO_STRUCT(update_window_set_immediate_cmd);
  update_window_set_immediate_cmd.cmd_len = 
    sizeof(update_window_set_immediate_cmd);
  update_window_set_immediate_cmd.cmd_id = 7;

  ZERO_STRUCT(update_window_enable_cmd);
  update_window_enable_cmd.cmd_len = sizeof(update_window_enable_cmd);
  update_window_enable_cmd.cmd_id = 13;

  ZERO_STRUCT(update_window_view_cmd);
  update_window_view_cmd.cmd_len = sizeof(update_window_view_cmd);
  update_window_view_cmd.cmd_id = 11;

  ZERO_STRUCT(update_window_position_cmd);
  update_window_position_cmd.cmd_len = sizeof(update_window_position_cmd);
  update_window_position_cmd.cmd_id = 10;
  
  ZERO_STRUCT(update_window_size_cmd);
  update_window_size_cmd.cmd_len = sizeof(update_window_size_cmd);
  update_window_size_cmd.cmd_id = 51;

  ZERO_STRUCT(change_resolution_cmd);
  change_resolution_cmd.cmd_len = sizeof(change_resolution_cmd);
  change_resolution_cmd.cmd_id = 50;

  ZERO_STRUCT(update_window_display_image_cmd);
  update_window_display_image_cmd.cmd_len = sizeof(update_window_display_image_cmd);
  update_window_display_image_cmd.cmd_id = 52;
}
  
void queue_command(void* cmd) {
  uint8_t* p = (uint8_t*) cmd;
  uint16_t* cmd_len = (uint16_t*)cmd;
  uint8_t* e = p + *cmd_len;
  for ( ; p < e; ++p) {
    *sdhr_data_p = *p;
  }
}

void queue_data(uint16_t len, void* data) {
  uint8_t* p = (uint8_t*) data;
  uint8_t* e = p + len;
  for ( ; p < e; ++p) {
    *sdhr_data_p = *p;
  }
}
		 
void disable_sdhr(void) {
  *sdhr_command_p = 0x00;
}

void enable_sdhr(void) {
  *sdhr_command_p = 0x01;
}

void process_commands(void) {
  *sdhr_command_p = 0x02;
}

void reset_sdhr(void) {
  *sdhr_command_p = 0x03;
}

uint16_t upload_file(const char* fn) {
  struct dir_block* dir_p;
  struct file_entry* e;
  uint8_t found;
  uint8_t ret;
  uint8_t i;
  uint8_t storage_type;
  uint8_t fn_len;
  uint16_t block_count;
  uint16_t current_block;
  //printf("upload_file %s\n",fn);
  dir_p = (struct dir_block*)dirbuf;
  dir_p->prev_next[1] = 2;
  found = 0;
  do {
    sp_load_block_source_lomed = dir_p->prev_next[1];
    sp_load_block_source_high = 0;
    sp_load_block_dest_addr = dirbuf;
    //printf("loading dir block %u\n",sp_load_block_source_lomed);
    ret = sp_load_block(dispatch_offset);
    if (ret) {
      BRK();
    }
    for (i = 0; i < 13; ++i) {
      //printf("file entry %u\n",i);
      e = dir_p->entries + i;
      storage_type = (e->storage_type_name_length & 0xf0) >> 4; 
      if (storage_type > 3) {
	continue;
      }
      fn_len = e->storage_type_name_length & 0x0f;
      if (fn_len != strlen(fn)) {
	continue;
      }
      if (memcmp(e->filename, fn, fn_len)) {
	continue;
      }
      found = 1;
      break;
    }
    if (found) break;
  } while (dir_p->prev_next[2] && dir_p->prev_next[3]);
  if (!found) {
    printf("NOT FOUND\n");
    BRK();
  }
  // determine block count
  if (e->eof[0]) {
    if (e->eof[1] == 255) {
      e->eof[1] = 0;
      ++e->eof[2];
    } else {
      ++e->eof[1];
    }
  }
  if (e->eof[1] & 0x01) {
    ++(e->eof[1]);
  }
  block_count = (e->eof[1] + e->eof[2]*256) / 2;
  current_block = 0;
  if (storage_type == 0x01) {
    process_seedling(e->block, &current_block);
  } else if (storage_type == 0x02) {
    process_sapling(e->block, block_count, &current_block);
  } else if (storage_type == 0x03) {
    process_tree(e->block, block_count, &current_block);
  } else {
    BRK();
  }
  return block_count;
}

void process_seedling(uint16_t block, uint16_t* current_block) {
  uint8_t ret;
  /* uint16_t i; */
  //printf("process_seedling %u %u\n", *current_block, block);
  if (block) {
    sp_load_block_source_lomed = block;
    sp_load_block_dest_addr = databuf;
    ret = sp_load_block(dispatch_offset);
    if (ret) {
      BRK();
    }
  } else {
    // sparse entry
    memset(&databuf, 0, sizeof(databuf));
  }
  /* for (i = 0; i < 16; ++i) { */
  /*   printf("%02x ",databuf[i]); */
  /*   if ( (i % 8) == 7) { */
  /*     printf("\n"); */
  /*   }  */
  /* } */
  /* printf("\n"); */
  /* BRK(); */
  upload_data_cmd.dest_block = *current_block;
  upload_data_cmd.source_addr = (uint16_t)databuf;
  queue_command(&upload_data_cmd);
  process_commands();
}

void process_sapling(uint16_t block, uint16_t block_count, uint16_t* current_block) {
  uint8_t ret;
  uint8_t i;
  uint16_t dest_block;
  //printf("process_sapling %u\n",block);
  sp_load_block_source_lomed = block;
  sp_load_block_dest_addr = indexbuf;
  ret = sp_load_block(dispatch_offset);
  if (ret) {
    BRK();
  }
  i = 0;
  do {
    dest_block = indexbuf[i+256] * 256 + indexbuf[i];
    process_seedling(dest_block, current_block);
    ++(*current_block);
    if (block_count == *current_block) {
      return;
    }
    ++i;
  } while (i != 0);
}


void process_tree(uint16_t block, uint16_t block_count, uint16_t* current_block) {
  uint8_t ret;
  uint8_t i;
  uint16_t dest_block;
  // printf("process_tree %u\n",block);
  sp_load_block_source_lomed = block;
  sp_load_block_dest_addr = masterbuf;
  ret = sp_load_block(dispatch_offset);
  if (ret) {
    BRK();
  }
  for (i = 0; i < 128; ++i) { // tree master blocks have max 128 entries
    dest_block = masterbuf[i+256] * 256 + masterbuf[i];
    // printf("process sapling dest block: %u from block: %u\n",dest_block,block);
    if (dest_block == 0)
	continue;
    process_sapling(dest_block, block_count, current_block);  
  }
}

void create_image_asset(uint8_t asset_id, uint16_t block_count) {
  define_image_asset_cmd.asset_id = asset_id;
  define_image_asset_cmd.block_count = block_count;
  queue_command(&define_image_asset_cmd);
}

void define_tileset(uint8_t tileset_index, uint8_t asset_index,
		    uint8_t num_entries, uint16_t xdim, uint16_t ydim,
		    uint16_t block_count) {
  define_tileset_cmd.tileset_index = tileset_index;
  define_tileset_cmd.asset_index = asset_index;
  define_tileset_cmd.num_entries = num_entries;
  define_tileset_cmd.xdim = xdim;
  define_tileset_cmd.ydim = ydim;
  define_tileset_cmd.block_count = block_count;
  queue_command(&define_tileset_cmd);
}

void define_tileset_set_immediate(uint8_t tileset_index, uint8_t asset_index,
		    uint8_t num_entries, uint16_t xdim, uint16_t ydim,
		    uint8_t* data) {
  define_tileset_set_immediate_cmd.tileset_index = tileset_index;
  define_tileset_set_immediate_cmd.asset_index = asset_index;
  define_tileset_set_immediate_cmd.num_entries = num_entries;
  define_tileset_set_immediate_cmd.xdim = xdim;
  define_tileset_set_immediate_cmd.ydim = ydim;
  define_tileset_set_immediate_cmd.cmd_len = 
    sizeof(define_tileset_set_immediate_cmd) + num_entries * 4;
  queue_command(&define_tileset_set_immediate_cmd);
  queue_data(num_entries * 4, data);
}

void define_window(uint8_t window_index, 
		   uint16_t screen_xcount, uint16_t screen_ycount,
		   uint16_t tile_xdim, uint16_t tile_ydim,
		   uint16_t tile_xcount, uint16_t tile_ycount) {
  define_window_cmd.window_index = window_index;
  define_window_cmd.screen_xcount = screen_xcount;
  define_window_cmd.screen_ycount = screen_ycount;
  define_window_cmd.tile_xdim = tile_xdim;
  define_window_cmd.tile_ydim = tile_ydim;
  define_window_cmd.tile_xcount = tile_xcount;
  define_window_cmd.tile_ycount = tile_ycount;
  queue_command(&define_window_cmd);
}

void update_window_set_upload(uint8_t window_index, uint16_t block_count) {
  update_window_set_upload_cmd.window_index = window_index;
  update_window_set_upload_cmd.block_count = block_count;
  queue_command(&update_window_set_upload_cmd);
}

void update_window_set_immediate(uint8_t window_index, 
				 uint16_t data_len, uint8_t* data) {
  update_window_set_immediate_cmd.window_index = window_index;
  update_window_set_immediate_cmd.data_len = data_len;
  queue_command(&update_window_set_immediate_cmd);
  queue_data(data_len, data);
}

void update_window_enable(uint8_t window_index, uint8_t enabled, uint32_t anim_ms_frame) {
  update_window_enable_cmd.window_index = window_index;
  update_window_enable_cmd.enabled = enabled;
  update_window_enable_cmd.anim_ms_frame = anim_ms_frame;
  queue_command(&update_window_enable_cmd);
}

void update_window_view(uint8_t window_index, 
			int32_t screen_xbegin, int32_t screen_ybegin) {
  update_window_view_cmd.window_index = window_index;
  update_window_view_cmd.screen_xbegin = screen_xbegin;
  update_window_view_cmd.screen_ybegin = screen_ybegin;
  queue_command(&update_window_view_cmd);
}

void update_window_position(uint8_t window_index, 
			    int32_t screen_xbegin, int32_t screen_ybegin) {
  update_window_position_cmd.window_index = window_index;
  update_window_position_cmd.screen_xbegin = screen_xbegin;
  update_window_position_cmd.screen_ybegin = screen_ybegin;
  queue_command(&update_window_position_cmd);
}

void update_window_size(uint8_t window_index, 
			    int32_t screen_xcount, int32_t screen_ycount) {
  update_window_size_cmd.window_index = window_index;
  update_window_size_cmd.screen_xcount = screen_xcount;
  update_window_size_cmd.screen_ycount = screen_ycount;
  queue_command(&update_window_size_cmd);
}

void change_resolution(uint32_t width, uint32_t height) {
  change_resolution_cmd.width = width;
  change_resolution_cmd.height = height;
  queue_command(&change_resolution_cmd);
}

void update_window_display_image(uint8_t window_index, uint8_t asset_index, uint8_t pixel_size) { 
  update_window_display_image_cmd.window_index = window_index;
  update_window_display_image_cmd.asset_index = asset_index;
  update_window_display_image_cmd.pixel_size = pixel_size;
  queue_command(&update_window_display_image_cmd);
}
