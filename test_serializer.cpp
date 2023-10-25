#include <stdint.h>
#include <stdlib.h>

typedef uint64_t u64;
typedef  int64_t i64;
typedef uint32_t u32;
typedef  int32_t i32;
typedef    float f32;
typedef   double f64;

#include "serializer.h"
#include "input.h"

int main()
{
	printf("testing...\n");
	char* storage = (char*)malloc(32 * 1024);
	Msg213 pack = {};
	char* end = json_serialize(pack, storage);
	*end = '\0';
	printf("%s\n", storage);
}