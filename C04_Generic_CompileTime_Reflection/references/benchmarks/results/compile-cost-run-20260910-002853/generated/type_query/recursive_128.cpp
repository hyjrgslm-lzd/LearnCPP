#include <type_traits>
        template<int I> struct tag {};

template<class T, class... Ts>
struct contains : std::false_type {};

template<class T, class Head, class... Tail>
struct contains<T, Head, Tail...>
    : std::conditional_t<std::is_same_v<T, Head>, std::true_type,
                         contains<T, Tail...>> {};

constexpr bool answer = contains<tag<127>, tag<0>, tag<1>, tag<2>, tag<3>, tag<4>, tag<5>, tag<6>, tag<7>, tag<8>, tag<9>, tag<10>, tag<11>, tag<12>, tag<13>, tag<14>, tag<15>, tag<16>, tag<17>, tag<18>, tag<19>, tag<20>, tag<21>, tag<22>, tag<23>, tag<24>, tag<25>, tag<26>, tag<27>, tag<28>, tag<29>, tag<30>, tag<31>, tag<32>, tag<33>, tag<34>, tag<35>, tag<36>, tag<37>, tag<38>, tag<39>, tag<40>, tag<41>, tag<42>, tag<43>, tag<44>, tag<45>, tag<46>, tag<47>, tag<48>, tag<49>, tag<50>, tag<51>, tag<52>, tag<53>, tag<54>, tag<55>, tag<56>, tag<57>, tag<58>, tag<59>, tag<60>, tag<61>, tag<62>, tag<63>, tag<64>, tag<65>, tag<66>, tag<67>, tag<68>, tag<69>, tag<70>, tag<71>, tag<72>, tag<73>, tag<74>, tag<75>, tag<76>, tag<77>, tag<78>, tag<79>, tag<80>, tag<81>, tag<82>, tag<83>, tag<84>, tag<85>, tag<86>, tag<87>, tag<88>, tag<89>, tag<90>, tag<91>, tag<92>, tag<93>, tag<94>, tag<95>, tag<96>, tag<97>, tag<98>, tag<99>, tag<100>, tag<101>, tag<102>, tag<103>, tag<104>, tag<105>, tag<106>, tag<107>, tag<108>, tag<109>, tag<110>, tag<111>, tag<112>, tag<113>, tag<114>, tag<115>, tag<116>, tag<117>, tag<118>, tag<119>, tag<120>, tag<121>, tag<122>, tag<123>, tag<124>, tag<125>, tag<126>, tag<127>>::value;
constexpr bool missing = contains<tag<128>, tag<0>, tag<1>, tag<2>, tag<3>, tag<4>, tag<5>, tag<6>, tag<7>, tag<8>, tag<9>, tag<10>, tag<11>, tag<12>, tag<13>, tag<14>, tag<15>, tag<16>, tag<17>, tag<18>, tag<19>, tag<20>, tag<21>, tag<22>, tag<23>, tag<24>, tag<25>, tag<26>, tag<27>, tag<28>, tag<29>, tag<30>, tag<31>, tag<32>, tag<33>, tag<34>, tag<35>, tag<36>, tag<37>, tag<38>, tag<39>, tag<40>, tag<41>, tag<42>, tag<43>, tag<44>, tag<45>, tag<46>, tag<47>, tag<48>, tag<49>, tag<50>, tag<51>, tag<52>, tag<53>, tag<54>, tag<55>, tag<56>, tag<57>, tag<58>, tag<59>, tag<60>, tag<61>, tag<62>, tag<63>, tag<64>, tag<65>, tag<66>, tag<67>, tag<68>, tag<69>, tag<70>, tag<71>, tag<72>, tag<73>, tag<74>, tag<75>, tag<76>, tag<77>, tag<78>, tag<79>, tag<80>, tag<81>, tag<82>, tag<83>, tag<84>, tag<85>, tag<86>, tag<87>, tag<88>, tag<89>, tag<90>, tag<91>, tag<92>, tag<93>, tag<94>, tag<95>, tag<96>, tag<97>, tag<98>, tag<99>, tag<100>, tag<101>, tag<102>, tag<103>, tag<104>, tag<105>, tag<106>, tag<107>, tag<108>, tag<109>, tag<110>, tag<111>, tag<112>, tag<113>, tag<114>, tag<115>, tag<116>, tag<117>, tag<118>, tag<119>, tag<120>, tag<121>, tag<122>, tag<123>, tag<124>, tag<125>, tag<126>, tag<127>>::value;
constexpr bool empty = contains<tag<127>>::value;

        static_assert(answer);
        static_assert(!missing);
        static_assert(!empty);
        extern "C" __declspec(dllexport) int result() { return (answer ? 100 : 0) + (missing ? 10 : 0) + (empty ? 1 : 0); }
        int main() { return result() == 100 ? 0 : 1; }
