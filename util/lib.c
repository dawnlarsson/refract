/*
        Dawning Experimental C standard library
        intended for tiny C programs that run in the distro
        without any runtime requirements (no linking!)

        Dawn Larsson - Apache-2.0 license
        github.com/dawnlarsson/dawning-linux/library/standard.c

        www.dawning.dev

        use the accompanying build shell script to compile
        $ sh build <source_file.c> <output_file_name>
*/

#ifndef DAWN_MODERN_C
#define DAWN_MODERN_C

#if defined(__clang__)

#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wstrict-prototypes"

#elif defined(__GNUC__) || defined(__GNUG__)

#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#endif

#if defined(__linux__) || defined(__unix__)
#define LINUX 1
#define UNIX 1
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define X64 1
#define BITS 64
#elif defined(__i386) || defined(_M_IX86)
#define X86 1
#define BITS 32
#elif defined(__aarch64__) || defined(_M_ARM64)
#define ARM64 1
#define BITS 64
#elif defined(__arm__) || defined(_M_ARM)
#define ARM32 1
#define BITS 32
#elif defined(__riscv)
#if __riscv_xlen == 64
#define RISCV64 1
#define BITS 64
#elif __riscv_xlen == 32
#define RISCV32 1
#define BITS 32
#endif
#elif defined(__PPC64__)
#define PPC64 1
#define BITS 64
#elif defined(__s390x__)
#define S390X 1
#define BITS 64
#endif

#if defined(__SSE__) || defined(__ARM_NEON)
#define SIMD 1
#endif

#if defined(__MODULE__) || defined(DAWN_MODERN_C_KERNEL)
#define KERNEL_MODE 1
#endif

#define FLAT __attribute__((flatten))
#define PURE __attribute__((pure))
#define NAKED __attribute__((naked))
#define INLINE __attribute__((always_inline))
#define NO_FRAME __attribute__((noframe))
#define KEEP __attribute__((used))
#define DEAD_END __attribute__((noreturn))
#define WEAK __attribute__((weak))

#define pub extern __attribute__((visibility("default"))) KEEP

#define address_to *
#define address_of &
#define address_any void *
#define address_bad ((address_any) - 1)

#undef null
#define null ((address_any)0)
#define null_ADDRESS null
#define is_null(address) ((address) == null)

// null terminator
#define end '\0'

typedef __builtin_va_list var_args;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
// ### Initializes a variable argument list.
#define var_list(list, last_param) __builtin_va_start(list, 0)
#else
// ### Initializes a variable argument list.
// list:        variable argument list to initialize
// last_param:  last named parameter before variable arguments
#define var_list(list, last_param) __builtin_va_start(list, last_param)
#endif

// ### Cleans up a variable argument list.
// list:        variable argument list to clean up
#define var_list_end(list) __builtin_va_end(list)

// Retrieves the next argument from a variable argument list.
// list:        variable argument list
// type:        type of the argument to retrieve
#define var_list_get(list, returned_type) __builtin_va_arg(list, returned_type)

// ### Creates a copy of a variable argument list.
// from:        source variable argument list to copy
// destination: variable argument list
#define var_list_copy(from, destination) __builtin_va_copy(destination, from)

// ### Convenience macro to process all variable arguments of a specific type.
// list:        variable argument list
// count:       number of arguments to process
// type:        type of arguments
// action:      Code block with '_arg' representing each argument
#define var_list_iter(list, count, type, action)              \
        do                                                    \
        {                                                     \
                for (int _i = 0; _i < (count); _i++)          \
                {                                             \
                        type _arg = var_list_get(list, type); \
                        action;                               \
                }                                             \
        } while (0)

#define bit_test(bit, address) (address_to(address) & (1u << (bit)))
#define bit_set(bit, address) (address_to(address) |= (1u << (bit)))
#define bit_clear(bit, address) (address_to(address) &= ~(1u << (bit)))
#define bit_flip(bit, address) (address_to(address) ^= (1u << (bit)))
#define bit_mask(bit) (1u << (bit))

#define struct_from_field(field_address, struct_type, field_name) \
        ((struct_type address_to)((b8 address_to)(field_address) - __builtin_offsetof(struct_type, field_name)))



// ### Values that never can be negative
#define positive_range unsigned

// ### Values that can be negative
#define bipolar_range signed

/// A non value returning function
typedef void fn;

// ### Positive range 8 bit integer
// range:       0 to +255
// memory:      [ 00000000 ]
// hex:         [ 0x00 ]
// linguistic:  (zero) to (plus) two hundred fifty-five
// traditional: unsigned char
// alt:         array of 8 bits
typedef positive_range char p8;
#define p8_max 255U
#define p8_min 0
#define p8_char_max 3
#define p8_bytes 1
#define p8_bits 8

// ### Bipolar range 8 bit integer
// range:       -128 to +127
// memory:      [ 00000000 ]
// hex:         [ 0x00 ]
// linguistic:  (minus) one hundred twenty-eight to (plus) one hundred twenty-seven
// traditional: char
// alt:         array of 8 bits
typedef bipolar_range char b8;
#define b8_max 127
#define b8_min -128
#define b8_char_max 4
#define b8_bytes 1
#define b8_bits 8

// ### Positive range 16 bit integer
// range:       0 to +65535
// memory:      [ 00000000 | 00000000 ]
// hex:         [ 0x00 | 0x00 ]
// linguistic:  (zero) to (plus) sixty-five thousand...
// traditional: unsigned short
// alt:         array of 16 bits
typedef positive_range short int p16;
#define p16_max 65535U
#define p16_min 0
#define p16_char_max 6
#define p16_bytes 2
#define p16_bits 16

// ### Bipolar range 16 bit integer
// range:       -32768 to +32767
// memory:      [ 00000000 | 00000000 ]
// hex:         [ 0x00 | 0x00 ]
// linguistic:  (minus) thirty-two thousand... to (plus) thirty-two thousand...
// traditional: short
// alt:         array of 16 bits
typedef bipolar_range short int b16;
#define b16_max 32767
#define b16_min -32768
#define b16_char_max 6
#define b16_bytes 2
#define b16_bits 16

// ### Positive range 32 bit integer
// range:       0 to +4294967295
// memory:      [ 00000000 | 00000000 | 00000000 | 00000000 ]
// hex:         [ 0x00 | 0x00 | 0x00 | 0x00 ]
// linguistic:  (zero) to (plus) four billion...
// traditional: unsigned int
// alt:         array of 32 bits
typedef positive_range int p32;
#define p32_max 4294967295U
#define p32_min 0
#define p32_char_max 10
#define p32_bytes 4
#define p32_bits 32

// ### Bipolar range 32 bit integer
// range:       -2147483648 to +2147483647
// memory:      [ 00000000 | 00000000 | 00000000 | 00000000 ]
// hex:         [ 0x00 | 0x00 | 0x00 | 0x00 ]
// linguistic:  (minus) two billion... to (plus) two billion...
// traditional: int
// alt:         array of 32 bits
typedef bipolar_range int b32;
#define b32_max 2147483647
#define b32_min (-2147483647 - 1)
#define b32_char_max 11
#define b32_bytes 4
#define b32_bits 32

// ### Positive range 64 bit integer
// range:       0 to +18446744073709551615
// memory:      [ 00000000 | 00000000 | 00000000 | 00000000 | 00000000 | 00000000 | 00000000 | 00000000 ]
// hex:         [ 0x00 | 0x00 | 0x00 | 0x00 | 0x00 | 0x00 | 0x00 | 0x00 ]
// linguistic:  (zero) to (plus) eighteen quintillion...
// traditional: unsigned long int
// alt:         array of 64 bits
typedef positive_range long long int p64;
#define p64_max 18446744073709551615ULL
#define p64_min 0
#define p64_char_max 20
#define p64_bytes 8
#define p64_bits 64

// ### Bipolar range 64 bit integer
// range:       -9223372036854775808 to +9223372036854775807
// memory:      [ 00000000 | 00000000 | 00000000 | 00000000 | 00000000 | 00000000 | 00000000 | 00000000 ]
// hex:         [ 0x00 | 0x00 | 0x00 | 0x00 | 0x00 | 0x00 | 0x00 | 0x00 ]
// linguistic:  (minus) nine quintillion... to (plus) nine quintillion...
// traditional: long int
// alt:         array of 64 bits
typedef bipolar_range long long int b64;
#define b64_max 9223372036854775807LL
#define b64_min (-9223372036854775807LL - 1)
#define b64_char_max 21
#define b64_bytes 8
#define b64_bits 64

#if BITS != 64
__extension__ typedef bipolar_range long long int i64;
__extension__ typedef positive_range long long int u64;
#endif

typedef float f32;
#define f32_max 3.40282346638528859812e+38F
#define f32_min 1.17549435082228750797e-38F
#define f32_epsilon 1.1920928955078125e-07F
#define f32_char_max 10
#define f32_bytes 4
#define f32_bits 32

typedef double f64;
#define f64_max 1.79769313486231570815e+308
#define f64_min 2.22507385850720138309e-308
#define f64_epsilon 2.22044604925031308085e-16
#define f64_char_max 20
#define f64_bytes 8
#define f64_bits 64\n\n#if BITS == 64

// ### Positive range [native] bit integer
// recommended type for most cases (especially for memory addresses, or loops)
// where native is the default integer size of the system architecture
// that's compiled for, for example,
//
// 64 bit systems: positive == p64
// range:       0 to +18446744073709551615
//
// 32 bit systems: positive == p32
// range:       0 to +4294967295
//
typedef p64 positive;
#define positive_max p64_max
#define positive_min p64_min
#define positive_char_max p64_char_max
#define positive_bytes p64_bytes
#define positive_bits p64_bits

// ### Bipolar range [native] bit integer
// recommended type for most cases (especially for memory addresses, or loops)
// where native is the default integer size of the system architecture
// that's compiled for, for example,
//
// 64 bit systems: bipolar == b64
// range:       -9223372036854775808 to +9223372036854775807
//
// 32 bit systems: bipolar == b32
// range:       -2147483648 to +2147483647
typedef b64 bipolar;
#define bipolar_max b64_max
#define bipolar_min b64_min
#define bipolar_char_max b64_char_max
#define bipolar_bytes b64_bytes
#define bipolar_bits b64_bits

// ### Native Decimal range floating point
// range:       1.7E-308 to 1.7E+308
typedef f64 decimal;
#define decimal_max f64_max
#define decimal_min f64_min
#define decimal_char_max f64_char_max
#define decimal_bytes f64_bytes
#define decimal_bits f64_bits

#else

// ### Positive range [native] bit integer
// recommended type for most cases (especially for memory addresses, or loops)
// where native is the default integer size of the system architecture
// that's compiled for, for example,
//
// 64 bit systems: positive == p64
// range:       0 to +18446744073709551615
//
// 32 bit systems: positive == p32
// range:       0 to +4294967295
//
typedef p32 positive;
#define positive_max p32_max
#define positive_min p32_min
#define positive_char_max p32_char_max
#define positive_bytes p32_bytes
#define positive_bits p32_bits

// ### Bipolar range [native] bit integer
// recommended type for most cases (especially for memory addresses, or loops)
// where native is the default integer size of the system architecture
// that's compiled for, for example,
//
// 64 bit systems: bipolar == b64
// range:       -9223372036854775808 to +9223372036854775807
//
// 32 bit systems: bipolar == b32
// range:       -2147483648 to +2147483647
typedef b32 bipolar;
#define bipolar_max b32_max
#define bipolar_min b32_min
#define bipolar_char_max b32_char_max
#define bipolar_bytes b32_bytes
#define bipolar_bits b32_bits

// ### Native Decimal range floating point

typedef f32 decimal;
#define decimal_max f32_max
#define decimal_min f32_min
#define decimal_char_max f32_char_max
#define decimal_bytes f32_bytes
#define decimal_bits f32_bits
#endif

typedef typeof(sizeof(0)) sized;

#define false 0
#define true 1

#undef bool
#define bool p8

#define ir(asm_args...) \
        asm volatile(asm_args)

#define b8_data(...) asm volatile(".byte " #__VA_ARGS__ "\n")
#define b16_data(...) asm volatile(".word " #__VA_ARGS__ "\n")
#define b32_data(...) asm volatile(".long " #__VA_ARGS__ "\n")
#define b64_data(...) asm volatile(".quad " #__VA_ARGS__ "\n")

#if ARM64
#define fn_asm(name, return_type, arguments...) \
        static return_type name(arguments)

#else
#define fn_asm(name, return_type, arguments...) \
        static NAKED return_type name(arguments)
#endif

#if X64
#define ASM(name) asm_x64_##name
#endif

#if ARM64
#define ASM(name) asm_arm64_##name
#endif

#if RISCV64
#define ASM(name) asm_riscv64_##name
#endif

#define asm_x64_add "add"
#define asm_x64_sub "sub"
#define asm_x64_copy "mov"
#define asm_x64_copy_64 "movq"
#define asm_x64_copy_32 "movl"
#define asm_x64_store "mov"
#define asm_x64_jump "jmp"
#define asm_x64_branch "je"
#define asm_x64_ret "ret"
#define asm_x64_reg_0 "%rax"
#define asm_x64_reg_1 "%rdi"
#define asm_x64_reg_2 "%rsi"
#define asm_x64_reg_3 "%rdx"
#define asm_x64_reg_4 "%rcx"
#define asm_x64_reg_5 "%r8"
#define asm_x64_reg_6 "%r9"
#define asm_x64_temp_0 "%r10"
#define asm_x64_temp_1 "%r11"
#define asm_x64_stack_pointer "%rsp"
#define asm_x64_frame_pointer "%rbp"
#define asm_x64_syscall "syscall"
#define asm_x64_syscall_slot asm_x64_reg_0

#define asm_arm64_add "add"
#define asm_arm64_sub "sub"
#define asm_arm64_copy "mov"
#define asm_arm64_jump "b"
#define asm_arm64_branch "beq"
#define asm_arm64_store "str"
#define asm_arm64_ret "ret"
#define asm_arm64_reg_0 "x0"
#define asm_arm64_reg_1 "x1"
#define asm_arm64_reg_2 "x2"
#define asm_arm64_reg_3 "x3"
#define asm_arm64_reg_4 "x4"
#define asm_arm64_reg_5 "x5"
#define asm_arm64_reg_6 "x6"
#define asm_arm64_temp_0 "x9"
#define asm_arm64_temp_1 "x10"
#define asm_arm64_stack_pointer "sp"
#define asm_arm64_frame_pointer "x29"

#if defined(MACOS) && defined(ARM64)
#define asm_arm64_syscall "svc 0x80"
#define asm_arm64_syscall_slot "x16"
#else
#define asm_arm64_syscall "svc 0"
#define asm_arm64_syscall_slot "x8"
#endif

#define asm_riscv64_add "add"
#define asm_riscv64_sub "sub"
#define asm_riscv64_copy "ld"
#define asm_riscv64_store "sd"
#define asm_riscv64_jump "jalr"
#define asm_riscv64_branch "beq"
#define asm_riscv64_syscall "ecall"
#define asm_riscv64_ret "ret"
#define asm_riscv64_reg_0 "a0"
#define asm_riscv64_reg_1 "a1"
#define asm_riscv64_reg_2 "a2"
#define asm_riscv64_reg_3 "a3"
#define asm_riscv64_reg_4 "a4"
#define asm_riscv64_reg_5 "a5"
#define asm_riscv64_reg_6 "a6"
#define asm_riscv64_temp_0 "t0"
#define asm_riscv64_temp_1 "t1"
#define asm_riscv64_stack_pointer "sp"
#define asm_riscv64_frame_pointer "s0"
#define asm_riscv64_syscall_slot asm_riscv64_reg_0

#define copy(where, from) ir(ASM(copy) " " ASM(where) "," ASM(from) ";")
#define jump(where) ir(ASM(jump) " " ASM(where) ";")
#define branch(where) ir(ASM(branch) " " ASM(where) ";")
#define add(what, with) ir(ASM(add) " " ASM(what) "," ASM(with) ";")
#define sub(what, with) ir(ASM(sub) " " ASM(what) "," ASM(with) ";")
#define system_return ir(ASM(ret))
#define system_invoke ir(ASM(syscall))
#define call(what) ir("call " ASM(what) ";")

#define register_get(reg, dest) ir(ASM(copy) " %0, " ASM(reg) : "=r"(dest))
#define register_set(reg, src) ir(ASM(copy) " " ASM(reg) ", %0" : : "r"(src))

// ### String address
// a pointer to a string in memory, usually the first p8 character of the string
typedef p8 address_to string_address;
typedef p8 string[];
typedef const p8 address_to const_string;

// Helper function for writing static strings to a writer with data + length
// example with:
//      write(str("Hello, world!\n"));
// example without:
//      write("Hello, world!\n", 14); // error prone!
#define str(string) (string), (sizeof(string) - 1)

#define string_index(source, index) (address_to((source) + (index)))
#define string_get(source) (address_to(source))
#define string_set(source, value) (address_to(source) = value)
#define string_is(source, value) (address_to(source) == (value))
#define string_not(source, value) (address_to(source) != (value))
#define string_equals(source, input) (string_compare(source, input) == 0)

#define string_set_if(source, check, value) \
        ((source) == (check) ? ((source) = (value), true) : false)

#undef min
#define min(value, input) ((value) > (input) ? (input) : (value))

#undef max
#define max(value, input) ((value) < (input) ? (input) : (value))

#define square(value) ((value) * (value))
#define cube(value) ((value) * (value) * (value))
#define mod(value, input) ((value) % (input))
#define floor(a) ((decimal)((bipolar)(a)))

#undef clamp
#define clamp(value, min, max) ((value) < (min) ? (min) : (value) > (max) ? (max) \
                                                                          : (value))

// Writer functions are intended as flexible outout functions passed to functions as arguments
// and should be easy for compiler to optimize into a zero cost abstraction
// if length is zero, the function should write until a null terminator is reached (string_length)
// writers redused file size, faster, and more flexible
typedef fn(address_to writer)(address_any data, positive length);
typedef fn(address_to writer_string)(string_address string);
typedef fn(address_to writer_string_len)(string_address string, positive length);

#ifndef DAWN_MODERN_C_NO_MATH

#define PI 3.14159265359f
#define PI2 6.28318530718f
#define PI_05x (PI * 0.5f)

#define CONVERSION_CONSTANTS                         \
        constexpr decimal RadToDeg = 180.0f / PI;    \
        constexpr decimal RadToTurn = 0.5f / PI;     \
        constexpr decimal DegToRad = PI / 180.0f;    \
        constexpr decimal DegToTurn = 0.5f / 180.0f; \
        constexpr decimal TurnToRad = PI / 0.5f;     \
        constexpr decimal TurnToDeg = 180.0f / 0.5f

#define AngleRad(value) (value)
#define AngleDeg(value) ((value) * DegToRad)
#define AngleTurn(value) ((value) * TurnToRad)

#ifndef KERNEL_MODE // for now :3
// simpler polynomial error < 0.01
decimal fast_sin(decimal x)
{
        x = x - PI2 * (bipolar)(x / PI2);

        if (x < 0)
                x += PI2;

        decimal sign = 1.0f;

        if (x > PI)
        {
                x -= PI;
                sign = -1.0f;
        }
        if (x > PI / 2)
        {
                x = PI - x;
        }

        return sign * 4.0f * x * (PI - x) / (PI * PI);
}
#endif // KERNEL_MODE

typedef union vector2
{
        struct
        {
                decimal x, y;
        };

        struct
        {
                decimal width, height;
        };

        decimal axis[2];

} vector2;

typedef union bipolar2
{
        struct
        {
                bipolar x, y;
        };

        struct
        {
                bipolar width, height;
        };

        bipolar axis[2];

} bipolar2;

typedef union positive2
{
        struct
        {
                positive x, y;
        };

        struct
        {
                positive width, height;
        };

        positive axis[2];

} positive2;

typedef union vector3
{
        struct
        {
                decimal x, y, z;
        };

        struct
        {
                decimal width, height, depth;
        };

        decimal axis[3];

} vector3;

typedef union bipolar3
{
        struct
        {
                bipolar x, y, z;
        };

        struct
        {
                bipolar width, height, depth;
        };

        bipolar axis[3];

} bipolar3;

typedef union positive3
{
        struct
        {
                positive x, y, z;
        };

        struct
        {
                positive width, height, depth;
        };

        positive axis[3];

} positive3;

typedef union vector4
{
        struct
        {
                decimal x, y, z, w;
        };

        struct
        {
                decimal width, height, depth, time;
        };

        decimal axis[4];

} vector4;

typedef vector4 quaternion;

typedef union matrix2
{
        decimal axis[2][2];
        vector2 colum[2];
} matrix2;

typedef union matrix3
{
        decimal axis[3][3];
        vector3 colum[3];
} matrix3;

typedef union matrix4
{
        decimal axis[4][4];
        vector4 colum[4];
} matrix4;

#endif // DAWN_MODERN_C_NO_MATH

// ### Fill a memory block with the same value
// fills a memory block with the same value
// returns: destination address
// destination: the memory block to fill
// traditional: memset
address_any memory_fill(address_any destination, b8 value, positive size)
{
        b8 address_to dest = (b8 address_to)destination;

        while (size--)
                address_to dest++ = (b8)value;

        return destination;
}

// ### Fill source memory block with destination memory block
// copies a memory block from source to destination
// returns: destination address
// destination: the memory block to copy to
// source: the memory block to copy from
// traditional: memcpy
address_any memory_copy(address_any destination, address_any source, positive size)
{
        b8 address_to dest = (b8 address_to)destination;
        b8 address_to src = (b8 address_to)source;

        // overlapping regions
        if (dest > src && dest < src + size)
        {
                dest += size - 1;
                src += size - 1;
                while (size--)
                        address_to dest-- = address_to src--;
        }
        else
        {
                while (size--)
                        address_to dest++ = address_to src++;
        }

        return destination;
}

// ### Fast memory copy
// copies a memory block from source to destination but dosn't handle overlapping regions
address_any memory_copy_fast(address_any destination, address_any source, positive size)
{
        b8 address_to dest = (b8 address_to)destination;
        b8 address_to src = (b8 address_to)source;

        while (size--)
                address_to dest++ = address_to src++;

        return destination;
}

// ### Length of string segment in linear memory
// returns the length of a string terminated by a null character
// NOT a entire array length
// a string array can hold more than one string, null terminators
// are used to separate strings, so where you run strlen is important
// traditional: strlen
positive string_length(string_address source)
{
        string_address step = source;

        while (string_get(step))
                step++;

        return step - source;
}

// ### Compare two string segments
// returns: 0 - if strings are equal
// returns: positive number - if first string is greater
// returns: negative number - if second string is greater
// traditional: strcmp
b32 string_compare(string_address source, string_address input)
{
        while (string_get(source) && string_get(input))
        {
                if string_not (source, address_to input)
                        break;

                source++;
                input++;
        }

        return string_get(source) - string_get(input);
}

// ### Copy string segment
// copies a string segment from source to destination
// returns: destination address
// destination: the memory block to copy to
// source: the memory block to copy from
// traditional: strcpy
string_address string_copy(string_address destination, string_address source)
{
        string_address start = destination;

        while (string_get(source))
                string_set(destination++, string_get(source++));

        string_set(destination, end);

        return start;
}

// ### Copy string segment with a maximum length
// traditional: strncpy
string_address string_copy_max(string_address destination, string_address source, positive length)
{
        string_address start = destination;

        while (length-- && string_get(source))
                string_set(destination++, string_get(source++));

        string_set(destination, end);

        return start;
}

// ### Find first character in string segment
// returns: address of the first occurrence of the character
// returns: null if the character is not found
// source: the memory block to search
// character: the character to search for
// traditional: strchr
string_address string_first_of(string_address source, p8 character)
{
        while (string_get(source))
        {
                if string_is (source, character)
                        return source;

                source++;
        }

        return (string_get(source) == character) ? source : null;
}

// ### Find last character in string segment
// returns: address of the last occurrence of the character
// returns: null if the character is not found
// source: the memory block to search
// character: the character to search for
// traditional: strrchr
string_address string_last_of(string_address source, p8 character)
{
        string_address last = null;

        while (string_get(source))
        {
                if string_is (source, character)
                        last = source;

                source++;
        }

        return last;
}

// Performs a single cut forward in a string by inserting a null terminator where the FIRST cut symbol is found.
// Returns the address AFTER the cut, effectively splitting the string into two parts.
// searching starts from the beginning of the string,
// linearly steps forward until a cut symbol is found or a string end is reached.
//
// # example:
//      string_address input = "Hello World";
//      string_address second_part = string_cut(input, ' ');
//      // input = "Hello"
//      // second_part = "World"
string_address string_cut(string_address string, b8 cut_symbol)
{
        string_address step = string;

        while (string_get(step))
        {
                if (string_is(step, cut_symbol))
                {
                        string_set(step, end);
                        step++;
                        return string_get(step) ? step : null;
                }
                step++;
        }

        return null;
}

// returns the the start address of the first occurrence input, null if not found
string_address string_find(string_address string, string_address input)
{
        string_address step = string;

        while (string_get(step))
        {
                if (string_not(step, string_get(input)))
                {
                        step++;
                        continue;
                }

                string_address find = step;
                string_address step_input = input;

                while (string_get(step_input))
                {
                        if string_not (step, string_get(step_input))
                                break;

                        step++;
                        step_input++;
                }

                if string_is (step_input, end)
                        return find;

                step = find + 1;
        }

        return null;
}

fn string_replace_all(string_address string, b8 cut_symbol, b8 replace_symbol)
{
        string_address step = string;

        while (string_get(step))
        {
                if string_is (step, cut_symbol)
                        string_set(step, replace_symbol);

                step++;
        }
}

// ### Takes a positive number and writes out the string representation
fn positive_to_string(writer write, positive number)
{
        if (number == 0)
                return write("0", 1);

        p8 digits[21];
        p8 address_to step = digits + 20;
        address_to step = end;

        while (number > 0)
        {
                address_to(--step) = '0' + (number % 10);
                number /= 10;
        }

        write(step, digits + 20 - step);
}

fn bipolar_to_string(writer write, bipolar number)
{
        if (number >= 0)
                return positive_to_string(write, (positive)number);

        write("-", 1);

        positive abs_number = 0 - (positive)number;
        positive_to_string(write, abs_number);
}

positive string_to_positive(string_address input)
{
        positive length = string_length(input);
        if (length == 0)
                return 0;

        positive result = 0;
        positive multiplier = 1;
        string_address step = input + length - 1;

        while (step >= input && string_get(step) >= '0' && string_get(step) <= '9')
        {
                result += (string_get(step) - '0') * multiplier;
                multiplier *= 10;
                step--;
        }

        return result;
}

bipolar string_to_bipolar(string_address input)
{
        if (string_get(input) == '-')
        {
                input++;
                return -string_to_positive(input);
        }

        return string_to_positive(input);
}

fn decimal_to_string(writer write, decimal value)
{
#ifndef KERNEL_MODE // Temporary

        if (value < 0)
        {
                write("-", 1);
                value = -value;
        }

        bipolar integer_part = (bipolar)value;
        decimal fraction_part = value - integer_part;

        bipolar_to_string(write, integer_part);

        write(".", 1);

        fraction_part *= 1000000;
        integer_part = (bipolar)fraction_part;

        if (integer_part < 100000)
                write("0", 1);
        if (integer_part < 10000)
                write("0", 1);
        if (integer_part < 1000)
                write("0", 1);
        if (integer_part < 100)
                write("0", 1);
        if (integer_part < 10)
                write("0", 1);

        bipolar_to_string(write, integer_part);
#endif
}

fn string_format(writer write, string_address format, ...)
{
        var_args args;
        var_list(args, format);

        string_address segment_start = format;
        positive index = 0;

        while (string_get(format))
        {
                if string_not (format, '%')
                {
                        format++;
                        index++;
                        continue;
                }

                if (index > 0)
                        write(segment_start, format - segment_start);

                format++;
                index = 0;

                switch (string_get(format))
                {
                case 'b':
                {
                        // todo: fix, long int breaks here...
                        int raw_value = var_list_get(args, int);
                        bipolar value = (bipolar)raw_value;
                        bipolar_to_string(write, value);
                        break;
                }
                case 'p':
                {
                        positive value = var_list_get(args, positive);
                        positive_to_string(write, value);
                        break;
                }
#ifndef KERNEL_MODE // Temporary
                case 'f':
                {
                        decimal value = var_list_get(args, decimal);
                        decimal_to_string(write, value);
                        break;
                }
#endif
                case 's':
                {
                        string_address value = var_list_get(args, string_address);
                        write(value, 0);
                        break;
                }
                case '%':
		        write("%", 1);
                        break;

                case end:
                        var_list_end(args);
                        return;
                }

                format++;
                segment_start = format;
        }

        write(segment_start, format - segment_start);

        var_list_end(args);
}

// ### Takes a path and writes out the last directory name
fn path_basename(writer write, string_address input)
{
        positive length = string_length(input);

        if (length == 0)
                return;

        while (length > 1 && input[length - 1] == '/')
                length--;

        if (length == 1 && input[0] == '/')
                return write("/", 1);

        positive step = length;

        while (step > 0 && input[step - 1] != '/')
                step--;

        write(input + step, length - step);
}

// ### Get CPU time (Time Stamp Counter)
// returns: the current CPU time
p64 get_cpu_time()
{
#if defined(X64)
        p32 high, low;
        ir("rdtsc" : "=a"(low), "=d"(high));
        return ((p64)high << 32) | low;
#elif defined(ARM64)
        p64 result;
        ir("mrs %0, cntvct_el0" : "=r"(result));
        return result;
#elif defined(RISCV64)
        p64 result;
        ir("rdtime %0" : "=r"(result));
        return result;
#else
        return 0;
#endif
}

// Userspace land
#ifndef KERNEL_MODE

// ### System call
// invokes operating system functions externally to the program
// returns: status code of the system call
// syscall: the system call number
fn_asm(system_call, bipolar, positive syscall)
{
        // syscall number
        copy(reg_0, syscall_slot);

        system_invoke;
        system_return;
}

// ### System call
// invokes operating system functions externally to the program
// returns: status code of the system call
// syscall: the system call number
fn_asm(system_call_1, bipolar, positive syscall, positive argument_1)
{
        // syscall number
        copy(reg_1, syscall_slot);

        // syscall argument
        copy(reg_2, reg_1);

        system_invoke;
        system_return;
}

// ### System call
// invokes operating system functions externally to the program
// returns: status code of the system call
// syscall: the system call number
fn_asm(system_call_2, bipolar, positive syscall, positive argument_1, positive argument_2)
{
        // syscall number
        copy(reg_1, syscall_slot);

        // syscall argument
        copy(reg_2, reg_1);

        // syscall argument
        copy(reg_3, reg_2);

        system_invoke;
        system_return;
}

// ### System call
// invokes operating system functions externally to the program
// returns: status code of the system call
// syscall: the system call number
fn_asm(system_call_3, bipolar, positive syscall, positive argument_1, positive argument_2, positive argument_3)
{
        // syscall number
        copy(reg_1, syscall_slot);

        // syscall argument
        copy(reg_2, reg_1);

        // syscall argument
        copy(reg_3, reg_2);

        // syscall argument
        copy(reg_4, reg_3);

        system_invoke;
        system_return;
}

// ### System call
// invokes operating system functions externally to the program
// returns: status code of the system call
// syscall: the system call number
fn_asm(system_call_4, bipolar, positive syscall, positive argument_1, positive argument_2, positive argument_3, positive argument_4)
{
        // syscall number
        copy(reg_1, syscall_slot);

        // syscall argument
        copy(reg_2, reg_1);

        // syscall argument
        copy(reg_3, reg_2);

        // syscall argument
        copy(reg_4, reg_3);

        // syscall argument
        copy(reg_5, reg_4);

        system_invoke;
        system_return;
}

// ### System call
// invokes operating system functions externally to the program
// returns: status code of the system call
// syscall: the system call number
fn_asm(system_call_5, bipolar, positive syscall, positive argument_1, positive argument_2, positive argument_3, positive argument_4, positive argument_5)
{
        // syscall number
        copy(reg_1, syscall_slot);

        // syscall argument
        copy(reg_2, reg_1);

        // syscall argument
        copy(reg_3, reg_2);

        // syscall argument
        copy(reg_4, reg_3);

        // syscall argument
        copy(reg_5, reg_4);

        // syscall argument
        copy(reg_6, reg_5);

        system_invoke;
        system_return;
}

// ### System call
// invokes operating system functions externally to the program
// returns: status code of the system call
// syscall: the system call number
fn_asm(system_call_6, bipolar, positive syscall, positive argument_1, positive argument_2, positive argument_3, positive argument_4, positive argument_5, positive argument_6)
{
        // syscall number
        copy(reg_1, syscall_slot);

        // syscall argument
        copy(reg_2, reg_1);

        // syscall argument
        copy(reg_3, reg_2);

        // syscall argument
        copy(reg_4, reg_3);

        // syscall argument
        copy(reg_5, reg_4);

        // syscall argument
        copy(reg_6, reg_5);

        // syscall argument
        copy(temp_0, reg_6);

        system_invoke;
        system_return;
}

typedef struct timespec
{
        p64 tv_sec;
        p64 tv_nsec;
} timespec;

// User required implementations
b32 main();

// Platform required implementations
fn exit(b32 code);
fn sleep(timespec address_to time);

#undef memset
// for compatibility, makes the linker happy
address_any memset(address_any destination, int value, long unsigned int size)
{
        return memory_fill(destination, value, size);
}

#ifndef DAWN_NO_PLATFORM

#define stdin 0
#define stdout 1
#define stderr 2

#define SIGTRAP 5
#define SIGKILL 9
#define SIGSTOP 20
#define SIGCHLD 17

#define O_NOCTTY 0400
#define O_NONBLOCK 0
#define O_DIRECTORY 0200000
#define AT_FDCWD -100
#define O_TRUNC 01000
#define O_CLOEXEC 02000000

#define FILE_READ 00
#define FILE_WRITE (01 | 0100 | 01000)
#define FILE_READ_WRITE 02
#define FILE_EXECUTE 010
#define FILE_APPEND (01 | 0100 | 02000)
#define FILE_CREATE 0100
#define FILE_TRUNCATE 0200

#define FILE_PROTECT_READ 0400
#define FILE_PROTECT_WRITE 0200

#define FILE_MAP_PRIVATE 01000
#define FILE_MAP_SHARED 02000
#define FILE_MAP_ANONYMOUS 04000

#define FILE_SEEK_SET 0
#define FILE_SEEK_CUR 1
#define FILE_SEEK_END 2

#include "platform/syscall.c"
#include "platform/any.c"

typedef struct
{
        p32 device;
        p64 inode;
        p32 protection;
        p64 hard_links;
        p32 owner;
        p32 group;
        p32 special_device_id;
        b64 size;
        b64 blocksize;
        b64 blocks;
        b64 last_access;
        b64 last_edit;
        b64 last_update;
} file_status;

typedef struct
{
        positive handle;
        string_address path;
        positive flags;
        address_any data;
        bool loaded;
        file_status status;
} file;

#define file_address file address_to

bool file_valid(file_address source)
{
        return source->handle != -1;
}

// flags: FILE_WRITE, FILE_READ, FILE_READ_WRITE, FILE_EXECUTE, FILE_TRUNCATE
fn file_new_lazy(file_address result, string_address path, positive flags)
{
        result->path = path;
        result->flags = flags;
        result->handle = system_call_3(syscall(openat), AT_FDCWD, (positive)path, flags);
}

// Get file status information, like size, last access time, etc.
//
// example usage:
//      file example = {0};
//      file_new_lazy(address_of example, "README.md", FILE_READ);
//      file_get_status(address_of example);
fn file_get_status(file_address source)
{
        system_call_2(syscall(fstat), source->handle, (positive)address_of source->status);
}

// file handle, path relative to the current working directory
// use file_new_lazy if you want to open a file without a status syscall
//
// flags: FILE_WRITE, FILE_READ, FILE_READ_WRITE, FILE_EXECUTE, FILE_TRUNCATE
//
// Examples:
//      file example = {0};
//
//      // open or create a file
//      file_new(address_to example, "vulkan.log", FILE_WRITE | FILE_CREATE | FILE_TRUNCATE);
//
//      // open a read only file *if* it exists
//      file_new(address_to example, "vulkan.log", FILE_READ);
//
//      // open a file for reading and writing, create it if it does not exist
//      file_new(address_to example, "vulkan.log", FILE_READ_WRITE | FILE_CREATE);
//
fn file_new(file_address result, string_address path, positive flags)
{
        file_new_lazy(result, path, flags);

        file_get_status(result);
}

// Load entire file into memory
address_any file_load(file_address source)
{
        if (!file_valid(source))
                return null;

        if (source->loaded && source->data)
                return source->data;

        positive size = source->status.size;

        if (size == 0)
                return null;

        positive page_size = 4096;
        positive pages = (size + page_size - 1) / page_size;

        source->data = (address_any)system_call_6(syscall(mmap),
                                                  0, pages * page_size,
                                                  FILE_PROTECT_READ | FILE_PROTECT_WRITE,
                                                  FILE_MAP_PRIVATE | FILE_MAP_ANONYMOUS, -1, 0);

        if (source->data == address_bad)
        {
                source->data = null;
                return null;
        }

        system_call_3(syscall(lseek), source->handle, 0, FILE_SEEK_SET);

        positive bytes_read = system_call_3(syscall(read), source->handle, (positive)source->data, size);

        if (bytes_read != size)
        {
                system_call_2(syscall(munmap), (positive)source->data, pages * page_size);
                source->data = null;
                return null;
        }

        source->loaded = true;
        return source->data;
}

positive file_read(file_address source, address_any buffer, positive size, positive offset)
{
        if (!file_valid(source))
                return -1;

        if (source->loaded && source->data)
        {

                if (offset >= source->status.size)
                        return 0;

                positive available = source->status.size - offset;
                positive to_read = size < available ? size : available;
                memory_copy(buffer, (p8 address_to)source->data + offset, to_read);
                return to_read;
        }

        system_call_3(syscall(lseek), source->handle, offset, FILE_SEEK_SET);
        return system_call_3(syscall(read), source->handle, (positive)buffer, size);
}

// Unload file data from memory
fn file_unload(file_address source)
{
        if (!source->data)
                return;

        positive page_size = 4096;
        positive pages = (source->status.size + page_size - 1) / page_size;

        system_call_2(syscall(munmap), (positive)source->data, pages * page_size);

        source->data = null;
        source->loaded = false;
}

// Write to file from provided buffer
positive file_write(file_address source, address_any buffer, positive size, positive offset)
{
        if (!file_valid(source))
                return -1;

        bool update_memory = source->loaded && source->data && offset < source->status.size;

        system_call_3(syscall(lseek), source->handle, offset, FILE_SEEK_SET);
        positive bytes_written = system_call_3(syscall(write), source->handle, (positive)buffer, size);

        if (update_memory && bytes_written > 0)
        {

                positive end_offset = offset + bytes_written;

                if (end_offset > source->status.size)
                {
                        file_get_status(source);
                        file_unload(source);
                }
                else
                {
                        memory_copy((p8 address_to)source->data + offset, buffer, bytes_written);
                }
        }

        return bytes_written;
}

// Close file and clean up resources
fn file_close(file_address source)
{
        if (!file_valid(source))
                return;

        file_unload(source);

        system_call_1(syscall(close), source->handle);
        source->handle = -1;
        source->path = null;
}

p8 working_directory[1024] = {0};

string_address working_directory_get()
{
        system_call_2(syscall(getcwd), (positive)working_directory, sizeof(working_directory));
        return working_directory;
}

fn working_directory_set(string_address path)
{
        system_call_1(syscall(chdir), (positive)path);
        working_directory_get();
}

#if defined(LINUX)
struct linux_dirent64
{
        p64 d_ino;
        p64 d_off;
        p16 d_reclen;
        p8 d_type;
        p8 d_name[];
};

positive2 term_size()
{
        positive2 size = {80, 24};

        struct
        {
                p16 rows;
                p16 cols;
                p16 xpixel;
                p16 ypixel;
        } data;

        if (!system_call_3(syscall(ioctl), 1, 0x5413, (positive)address_of data))
        {
                size.width = data.cols;
                size.height = data.rows;
        }

        return size;
}

fn sleep(timespec address_to time)
{
        system_call_3(syscall(nanosleep), (positive)time, 0, 0);
}

fn exit(b32 code)
{
        system_call_1(syscall(exit), code);
}

fn _start()
{
        b32 result = main();

        exit(result);
}
#endif

// ### Memory allocation
// Allocates a linear chunk of memory of the specified size.
address_any memory(positive size)
{
        if (size < 4096)
        {
                // tbd: bump allocator
        }

        return (address_any)system_call_6(syscall(mmap), 0, size,
                FILE_PROTECT_READ | FILE_PROTECT_WRITE,
                FILE_MAP_PRIVATE | FILE_MAP_ANONYMOUS, -1, 0);
}

fn memory_free(address_any address, positive size)
{
        if (!address || size == 0)
                return;

        system_call_2(syscall(munmap), (positive)address, size);
}

#endif // DAWN_NO_PLATFORM

#endif // KERNEL_MODE

#endif // DAWN_MODERN_C