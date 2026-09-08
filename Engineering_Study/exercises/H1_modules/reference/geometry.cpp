module geometry;
import :units;

int square_area(int side) {
    return side * side * unit_scale();
}
