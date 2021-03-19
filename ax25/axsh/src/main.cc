#include <iostream>
#include <stdexcept>

int wrapmain(int argc, char** argv);

int main(int argc, char** argv)
{
    try {
        return wrapmain(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception!\n";
        throw;
    }
}
