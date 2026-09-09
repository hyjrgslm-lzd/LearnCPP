#include <iostream>

int add_offset(int seed, int offset)
{
    int adjusted = seed + offset;
    return adjusted;
}

int compute_answer(int seed)
{
    int offset = 2;
    int adjusted = add_offset(seed, offset);
    int answer = adjusted * 2;
    return answer;
}

int main()
{
    int seed = 19;
    int answer = compute_answer(seed);
    std::cout << answer << '\n';
    return answer == 42 ? 0 : 1;
}
