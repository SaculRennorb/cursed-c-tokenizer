# Cursed C  Tokenizer

This is a small simple c tokenizer, mainly for processing structs.

The most part of this is the subsequent _incredibly cursed_ json serializer that generates serialization methods from the token stream.

This is by no means to any spec, its was just an experiment.
From 
```c
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
```
it will generate something like this:
```c
#include "input.h"
#include <stdio.h>

char* json_serialize(Msg222 const& packet, char* destination) {
	destination += sprintf(destination, R"-({
	"Type": "Msg222")-");
	destination += sprintf(destination, ",");
	destination += sprintf(destination, R"-(
	"field1": %u)-", packet.field1);
	destination += sprintf(destination, ",");
	destination += sprintf(destination, R"-(
	"field2": %d)-", packet.field2);
	destination += sprintf(destination, R"(
	})");
	return destination;
}

char* json_serialize(Msg213 const& packet, char* destination) {
	destination += sprintf(destination, R"-({
	"Type": "Msg213")-");
	destination += sprintf(destination, ",");
	destination += sprintf(destination, R"-(
	"field1": %llu)-", packet.field1);
	destination += sprintf(destination, ",");
	destination += sprintf(destination, R"-(
	"field2": %u)-", packet.field2);
	destination += sprintf(destination, ",");
	destination += sprintf(destination, R"-(
	"Inner1": )-");
	destination += sprintf(destination, "{");
	destination += sprintf(destination, R"-(
	"field3": %f)-", packet.Inner1.field3);
	destination += sprintf(destination, ",");
	destination += sprintf(destination, R"-(
	"field4": %u)-", packet.Inner1.field4);
	destination += sprintf(destination, R"(
	})");
	destination += sprintf(destination, ",");
	destination += sprintf(destination, R"-(
	"Inner2": )-");
	destination += sprintf(destination, "{");
	destination += sprintf(destination, R"-(
	"field3": %f)-", packet.Inner2.field3);
	destination += sprintf(destination, ",");
	destination += sprintf(destination, R"-(
	"field4": %u)-", packet.Inner2.field4);
	destination += sprintf(destination, R"(
	})");
	destination += sprintf(destination, R"(
	})");
	return destination;
}

```

As this is only a tokenizer it has no concept of type reuse, so 
```c
struct A {
	u32 a;
};

struct B {
	A a;
};
```
will not work.