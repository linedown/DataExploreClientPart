#include <iostream>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <clocale>
#include <thread>
#include <chrono>
#include <cstdint>
#pragma comment(lib, "Ws2_32.lib")

#define IPPROTO_LAN 63

using namespace std;

const int messages = 10;
const int dataLength = 1024;
const int packetLength = 2048;

struct iphdr
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8_t ihl : 4;
    uint8_t version : 4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    uint8_t version : 4;
    uint8_t ihl : 4;
#else
# error	"Please fix <bits/endian.h>"
#endif
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
    /*The options start here. */
};

unsigned short checksum(void* b, int len) {
    unsigned short* buf = static_cast<unsigned short*>(b);
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;

    if (len == 1)
        sum += static_cast<unsigned char>(*buf);

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    result = static_cast<unsigned short>(~sum);
    return result;
}

void clientFunction() {
    WSAData wd;
    if (WSAStartup(MAKEWORD(2, 2), &wd) != 0) {
        cerr << "Ошибка при инициализации винсока: " << WSAGetLastError() << "\n";
        exit(1);
    }
    else cout << "Винсок проинициализирован успешно!!\n";

    int rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_LAN);
    if (rawSocket < 0) {
        cerr << "Ошибка при создании сокета " << WSAGetLastError() << "\n";
        WSACleanup();
        exit(1);
    }
    else cout << "Сокет создан успешно!!\n";

    struct sockaddr_in clientAddr;
    memset(&clientAddr, 0, sizeof clientAddr);
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = 0;

    if (inet_pton(AF_INET, "192.168.88.44", &clientAddr.sin_addr) < 0) {
        cerr << "Ошибка при переводе адреса: " << WSAGetLastError() << "\n";
        closesocket(rawSocket);
        WSACleanup();
        exit(1);
    }
    /*else cout << "Адрес переведен успешно!!\n";*/

    if (bind(rawSocket, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
        cerr << "Ошибка привязки сокета: " << WSAGetLastError() << "\n";
        closesocket(rawSocket);
        WSACleanup();
        exit(1);
    }
    else cout << "Привязка сокета прошла успешно!!\n";

    cout << "Клиент принимает данные..." << "\n";

    char packetBuffer[packetLength];
    struct sockaddr_in serverAddr;
    int serverAddrSize = sizeof serverAddr;

    for (int i = 0; i < messages; ++i) {
        memset(packetBuffer, 0, sizeof packetBuffer);

        int bytesAccept = recvfrom(rawSocket, packetBuffer, sizeof packetBuffer, 0, (struct sockaddr*)&serverAddr, &serverAddrSize);
        if (bytesAccept < 0) {
            cout << "Проблема с принятием пакета: " << WSAGetLastError() << "\n";
            continue;
        }
        else {
            cout << "Пакет " << i + 1 << " получен. Количество байт : " << bytesAccept << "\n";
            //cout << "Пакет получен. Количество байт: " << bytesAccept << " от " << inet_ntoa(clientAddr.sin_addr) << "\n";
            cout << "Пакет в виде массива символов: \n";
            for (char i : packetBuffer) cout << i;
            cout << "\n";
            iphdr* ipheader = (iphdr*)(packetBuffer);
            cout << "Версия протокола IP: " << static_cast<int>(ipheader->version) << "\n";
            cout << "Длина заголовка: " << static_cast<int>(ipheader->ihl) << "\n";
            cout << "Чек-сумма: " << ntohs(ipheader->check) << "\n";
            cout << "Флаги заголовка: " << ipheader->frag_off << "\n";
            cout << "ToS: " << static_cast<int>(ipheader->tos) << "\n";
            cout << "TTL: " << static_cast<int>(ipheader->ttl) << "\n";
            cout << "Версия протокола: " << static_cast<int>(ipheader->protocol) << "\n";
            cout << "Общая длина: " << ipheader->tot_len << "\n";

            if (ipheader->version != 4) {
                cout << "Неправильная версия протокола!\n";
                continue;
            }
            if (ipheader->protocol != IPPROTO_LAN) {
                cout << "Не тот протокол! (Ожидался " << IPPROTO_LAN << ", получен " << (int)ipheader->protocol << ")\n";
                //continue;
            }

            char* data = packetBuffer + sizeof(iphdr);
            cout << "Длина: " << dataLength << "\n";
            cout << "Длина принятых байт без длины заголовка: " << bytesAccept - sizeof(iphdr) << "\n";
            cout << "\nДанные: \n";
            for (int j = 0; j < bytesAccept - sizeof(iphdr); ++j) {
                cout << data[j];

            }
            cout << "\n";
        }
    }

    closesocket(rawSocket);
    WSACleanup();
}

int main() {
    setlocale(LC_ALL, "ru");
    clientFunction();

    cout << "Конец программы\n";
    return 0;
}
