import "ifj25" for Ifj
class Program {
    // funkce bez parametru
    static unicorn {
        // __a je globalni promenna. Pokud neni zatim definovana, implicitne je hodnota null.
        // null se v podmince chova jako false.
        if (20 > 10) {
            return __a + 10
        } else {
            return __a - 10
        }
    }
}
