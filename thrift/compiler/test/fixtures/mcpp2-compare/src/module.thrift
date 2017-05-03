namespace cpp2 some.valid.ns

// Generate base consts
const bool aBool = true
const byte aByte = 1
const i16 a16BitInt = 12
const i32 a32BitInt = 123
const i64 a64BitInt = 1234
const float aFloat = 0.1
const double aDouble = 0.12
const string aString = "Joe Doe"
const list<bool> aList = [true, false]
const map<i32, string> aMap = {
  1: "foo",
  2: "bar",
}
const set<string> aSet = ["foo", "bar"]
const list<list<i32>> aListOfLists = [
  [1, 3, 5, 7, 9],
  [2, 4, 8, 10, 12]
]
const list<map<string, i32>> states = [
  {"San Diego": 3211000, "Sacramento": 479600, "SF": 837400},
  {"New York": 8406000, "Albany": 98400}
]

struct Empty {
}

struct MyStruct {
  1: bool MyBoolField,
  2: i64 MyIntField = 12,
  3: string MyStringField = "test"
  4: string MyStringField2
}

typedef i32 simpleTypeDef
typedef map<i16, string> containerTypeDef
typedef list<map<i16, string>> complexContainerTypeDef
typedef list<MyStruct> structTypeDef
typedef list<map<Empty, MyStruct>> complexStructTypeDef
typedef list<complexStructTypeDef> mostComplexTypeDef

struct containerStruct {
  1: bool fieldA
  2: map<string, bool> fieldB
  3: set<i32> fieldC = [1, 2, 3, 4]
  4: string fieldD
  5: string fieldE = "somestring"
  6: list<list<i32>> fieldF = aListOfLists
  7: map<string, map<string, map<string, i32>>> fieldG
  8: list<set<i32>> fieldH
  9: bool fieldI = true
  10: map<string, list<i32>> fieldJ = {
       "subfieldA" : [1, 4, 8, 12],
       "subfieldB" : [2, 5, 9, 13],
     }
  11: list<list<list<list<i32>>>> fieldK
  12: set<set<set<bool>>> fieldL
  13: map<set<list<i32>>, map<list<set<string>>, string>> fieldM
}
