#pragma once

// comment here
struct Msg222 {
	u32 field1;
	i32 field2;
};

struct Msg213 {
	u64 field1;
	u32 field2;
	struct { // comment here
		f32 field3;
		u32 field4;
	} /* comment here */ Inner1;
	struct {
		f32 field3;
		u32 field4;
	} Inner2;
};