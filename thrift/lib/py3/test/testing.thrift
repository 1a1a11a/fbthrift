const list<i16> int_list = [
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
];

typedef list<i32> I32List
typedef bool Bool
typedef i64 TimeStamp
typedef byte Byte
typedef float Float
typedef double Double

enum Color {
  red = 0,
  blue = 1,
  green = 2,
}

service TestingService {
    i32 complex_action(1: string first, 2: string second, 3: i64 third, 4: string fourth)
    void takes_a_list(1: I32List ints)
    void pick_a_color(1: Color color)
    void int_sizes(1: byte one, 2: i16 two, 3: i32 three, 4: i64 four)
}
