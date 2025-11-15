import "ifj25" for Ifj
class Program {
    // funkce bez parametru
    static getAnswer() {
        return 42  // prikaz navratu z funkce
    }

    // pretizena verze funkce vyse s jednim parametrem
    static getAnswer(arg) {
        if (arg is Num) {   // podmineny prikaz, operator porovnani typu
            if (arg - 1 > 32 * 84.1 / 2.4) {  // porovnavaci vyraz ma na obou stranach zase vyrazy
<<<<<<< HEAD
                // v zakladnim zadani neumime volat funkci mimo prirazeni, proto zde prirazujeme
                // navratovou hodnotu (kazda funkce nejakou ma) napr. do "dummy" globalni promenne __d
                __d = Ifj.write("Celkem velke cislo!\n")
            } else {
                
            }
            // arg = arg * 42  // prikaz prirazeni
=======
                Ifj.write("Celkem velke cislo!\n")
            }
            arg = arg * 42  // prikaz prirazeni
>>>>>>> main
            var ansStr 
            ansStr = Ifj.str(arg)  // volani vestavene funkce
            return "Odpoved je " + ansStr
        } else {  // else sekce je v zakladnim zadani povinna
<<<<<<< HEAD
            __d = Ifj.write("Neplatny argument\n")
=======
            Ifj.write("Neplatny argument\n")
>>>>>>> main
            return null
        }
    }

    // staticky getter -> vraci hodnotu
    static unicorn {
        // __a je globalni promenna. Pokud neni zatim definovana, implicitne je hodnota null.
        // null se v podmince chova jako false.
        if (__a) {
            return __a + 10
        } else {
            return null
        }
    }

    // staticky setter -> chova se jako funkce, muze mit vedlejsi efekty, ale pristupuje se k ni jinak (viz nize)
    static unicorn=(val) {  
<<<<<<< HEAD
        __d = Ifj.write("Jsem jednorozci setter, ziskal jsem ")
        __d = Ifj.write(val)
        __d = Ifj.write("\n")
        // __a = val
=======
        Ifj.write("Jsem jednorozci setter, ziskal jsem ")
        Ifj.write(val)
        Ifj.write("\n")
        __a = val
>>>>>>> main
    }

    // jina funkce se muze jmenovat stejne jako getter, nema s nim nic spolecneho!
    static unicorn() {
<<<<<<< HEAD
        __d = Ifj.write("Jsem ve funkci unicorn, ne v getteru\n")
=======
        Ifj.write("Jsem ve funkci unicorn, ne v getteru\n")
>>>>>>> main
    }

    // hlavni funkce main
    static main() {
<<<<<<< HEAD
        __d = Ifj.write(unicorn) /* Staticky getter muze byt parametr ve volani funkce.
                                  * Tohle tedy prvne zavola a vyhodnoti telo getteru unicorn,
                                  * vysledek to pak pouzije jako parametr pro zavolani Ifj.write.
                                  * Toto volani skonci vypisem "null". */
        __d =  Ifj.write("\n")
=======
        Ifj.write(unicorn) /* Staticky getter muze byt parametr ve volani funkce.
                            * Tohle tedy prvne zavola a vyhodnoti telo getteru unicorn,
                            * vysledek to pak pouzije jako parametr pro zavolani Ifj.write.
                            * Toto volani skonci vypisem "null". */
        Ifj.write("\n")
>>>>>>> main

        unicorn = 5 /* Zavola telo setteru -> 
                     * ten vypise zpravu "Jsem jednorozci setter, ziskal jsem 5\n" 
                     * a nastavi glob. promennou __a na 5.
                     * Mimochodem /* viceradkove komentare mohou byt /* vnorene */, hura. */ */

        var myValue         // definice promenne - v zakladnim zadani neni s prirazenim (var myValue = ...), 
                            // vychozi hodnota je null
<<<<<<< HEAD
        // myValue = unicorn   // zavola telo getteru, ten vrati 15 (== __a + 10)
        __d = Ifj.write(myValue)  // vypise 15
        __d = Ifj.write("\n")

        myValue = unicorn()  // zavola funkci, ta vypise "Jsem ve funkci unicorn, ne v getteru\n"
        __d = Ifj.write(myValue)   // vypise null (funkce neurcila navratovou hodnotu, implicitne null)
        __d = Ifj.write("\n")

        __d = Ifj.write("Napis cislo:\n")
=======
        myValue = unicorn   // zavola telo getteru, ten vrati 15 (== __a + 10)
        Ifj.write(myValue)  // vypise 15
        Ifj.write("\n")

        myValue = unicorn()  // zavola funkci, ta vypise "Jsem ve funkci unicorn, ne v getteru\n"
        Ifj.write(myValue)   // vypise null (funkce neurcila navratovou hodnotu, implicitne null)
        Ifj.write("\n")

        Ifj.write("Napis cislo:\n")
>>>>>>> main
        var valueFromUser
        valueFromUser = Ifj.read_num()
        myValue = getAnswer(valueFromUser)  // zavola pretizenou funkci getAnswer(arg)
        if (myValue == null) {
<<<<<<< HEAD
            __d = Ifj.write("ODPOVED NENI!\n")
=======
            Ifj.write("ODPOVED NENI!\n")
>>>>>>> main
            return  // konec programu
        } else {
            if (myValue is Null) {
                // tato podminka je semanticky ekvivalentni podmince myValue == null,
                // sem se proto uz nikdy nedostaneme
            } else {
            }
        }
<<<<<<< HEAD
        __d = Ifj.write(myValue)
        __d = Ifj.write("\n")

        while (valueFromUser > 0 - 2) {  // prikaz cyklu; nemame zaporne literaly, takze -2 napiseme jako (0-2)
            __d = Ifj.write("Jedeme dolu! ")
            __d = Ifj.write(valueFromUser)  // pokud byla zadana hodnota desetinna, vypise ji v hexadecimalnim formatu
            __d = Ifj.write(" -> ")
            // valueFromUser = valueFromUser - 1
=======
        Ifj.write(myValue)
        Ifj.write("\n")

        while (valueFromUser > 0 - 2) {  // prikaz cyklu; nemame zaporne literaly, takze -2 napiseme jako (0-2)
            Ifj.write("Jedeme dolu! ")
            Ifj.write(valueFromUser)  // pokud byla zadana hodnota desetinna, vypise ji v hexadecimalnim formatu
            Ifj.write(" -> ")
            valueFromUser = valueFromUser - 1
>>>>>>> main

            var valStr 
            valStr = Ifj.str(valueFromUser)  /* pokud byla zadana hodnota desetinna, 
                                              * vypise ji v decimalnim formatu se dvema desetinnymi misty */
<<<<<<< HEAD
            __d = Ifj.write(valStr)
            __d = Ifj.write("\n")
=======
            Ifj.write(valStr)
            Ifj.write("\n")
>>>>>>> main
        }

        // Rozsahy platnosti:
        {  // na miste prikazu muze stat samostatny blok
            var x 
            x = 1
            {
                // Na tomto miste muzeme znovu definovat promennou s nazvem "x",
                // protoze kazdy blok vytvari novy rozsah platnosti (scope)!
                // V zakladnim zadani se od nasledujiciho radku nedostaneme k puvodni hodnote x (1) az do konce bloku
                // (v rozsireni EXTSTAT by ale bylo mozne napsat var x = x)
                var x
                x = "cervena panda + "
<<<<<<< HEAD
                __d = Ifj.write("(")
                if (1 == 1) {
                    var y 
                    // y = x * 3  // "nasobeni" rezetce celym cislem => iterace retezce (3krat se pripoji sam za sebe)
                    // Telo v podminenem prikazu je taky blok -> taky vytvari rozsah platnosti
                    var x
                    // x = y
                    __d = Ifj.write(x)
                } else {
                }
                __d = Ifj.write("konec) ")
                __d = Ifj.write(x)  // x = "cervena panda + "
            }
            __d = Ifj.write(x)  // x = 1
        }
        // __d = Ifj.write(x)  // nelze, zde uz x neexistuje
        __d = Ifj.write("\n")
=======
                Ifj.write("(")
                if (1 == 1) {
                    var y 
                    y = x * 3  // "nasobeni" rezetce celym cislem => iterace retezce (3krat se pripoji sam za sebe)
                    // Telo v podminenem prikazu je taky blok -> taky vytvari rozsah platnosti
                    var x
                    x = y
                    Ifj.write(x)
                } else {
                }
                Ifj.write("konec) ")
                Ifj.write(x)  // x = "cervena panda + "
            }
            Ifj.write(x)  // x = 1
        }
        // Ifj.write(x)  // nelze, zde uz x neexistuje
        Ifj.write("\n")
>>>>>>> main
    }
}
