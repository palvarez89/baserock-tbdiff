#ifndef __otap_error_h__
#define __otap_error_h__

typedef enum {
	otap_error_success =  0,
	otap_error_failure = -1,
	otap_error_out_of_memory = -2,
	otap_error_null_pointer = -3,
	otap_error_invalid_parameter = -4,
	otap_error_unable_to_read_stream  = -5,
	otap_error_unable_to_write_stream = -6,
	otap_error_unable_to_create_dir = -7,
	otap_error_unable_to_change_dir = -8,
	otap_error_unable_to_open_file_for_reading = -9,
	otap_error_unable_to_open_file_for_writing = -10,
	otap_error_file_already_exists = -11,
	otap_error_unable_to_remove_file = -12,
	otap_error_unable_to_seek_through_stream = -13,
	otap_error_feature_not_implemented = -14,
	otap_error_file_does_not_exist = -15,
	otap_error_unable_to_detect_stream_position = -16,
	otap_error_unable_to_stat_file = -17,
} otap_error_e;

#ifdef NDEBUG
#define otap_error(e) return e
#else
#define otap_error(e) { if(e != 0) fprintf(stderr, "OTAP error '%s' in function '%s' at line %d of file '%s'.\n", #e, __FUNCTION__, __LINE__, __FILE__); return e; }
#endif

#endif
