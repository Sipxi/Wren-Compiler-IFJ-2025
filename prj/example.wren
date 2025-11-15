import "ifj25" for Ifj
class Program {
    // funkce bez parametru
    

    static getAnswer(arg) {
        if (arg is Num) {   // podmineny prikaz, operator porovnani typu
             // porovnavaci vyraz ma na obou stranach zase vyrazy
                // v zakladnim zadani neumime volat funkci mimo prirazeni, proto zde prirazujeme
                // navratovou hodnotu (kazda funkce nejakou ma) napr. do "dummy" globalni promenne __d
            
            
            arg = arg * 42  // prikaz prirazeni
            var ansStr 
            ansStr = Ifj.str(arg)  // volani vestavene funkce
            return "Odpoved je " + ansStr
        } else {  // else sekce je v zakladnim zadani povinna
            __d = Ifj.write("Neplatny argument\n")
            return null
        }
    }
}
