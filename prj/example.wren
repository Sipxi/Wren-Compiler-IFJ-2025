import "ifj25" for Ifj
class Program {
    // funkce bez parametru
    

    // staticky setter -> chova se jako funkce, muze mit vedlejsi efekty, ale pristupuje se k ni jinak (viz nize)
    static unicorn=(val) {  
        __d = Ifj.write("Jsem jednorozci setter, ziskal jsem ")
        __d = Ifj.write(val)
        __d = Ifj.write("\n")
        __a = val
    }
    
}
