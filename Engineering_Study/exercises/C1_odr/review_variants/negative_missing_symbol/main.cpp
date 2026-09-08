int call_lesson_from_a();
int call_lesson_from_b();

int main()
{
    return call_lesson_from_a() == 42 && call_lesson_from_b() == 42 ? 0 : 1;
}
