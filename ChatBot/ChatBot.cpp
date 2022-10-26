#include <iostream>
#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct UserData
{
    int id;
    std::string name;
};

int main()
{
    const int start_id = 10;
    int latest_user_id = start_id;
    uWS::App().get("/hello", 
        [](auto* res, auto* req) 
        {
            res->writeHeader("Content-Type", "text/html; charset=utf-8")->end("Welcome to SmartChatBot!");
        }
    ).ws<UserData>("/*", {
        // Вызывается при подключении нового пользователя
        .open = [&latest_user_id](auto* ws) 
        { 
            UserData* user_data = ws->getUserData();
            //Выдать пользователю уникальный id
            user_data->id = latest_user_id++;
            //По-умолчанию новый пользователь будет "unnamed user"
            user_data->name = "unnamed user";

            std::cout << "User connected: " << user_data->id << std::endl;
            std::cout << "Total users connected: " << latest_user_id - start_id << std::endl;
            ws->subscribe("public_chanel"); //Подписываем всех пользователей на канал public_channel
            ws->subscribe("user" + std::to_string(user_data->id)); //еще и на личный канал
        },
        // Вызывается, когда от пользователя приходит какое-то сообщение (байтики)
        .message = [](auto* ws, std::string_view message, uWS::OpCode opCode) 
        {
            UserData* user_data = ws->getUserData(); //Получаем информацию о пользователе
            json message_data = json::parse(message); //Парсим данные от пользователя
            if (message_data["command"] == "public_msg")
            {
                json payload; //готовим данные
                //Отправляем публичное сообщение
                payload["command"] = "public_msg";
                payload["text"] = message_data["text"];
                payload["user_id"] = user_data->id;
                ws->publish("public_chanel", payload.dump());
                std::cout << "User sent public message: " << user_data->id << std::endl;

            }
            if (message_data["command"] == "private_msg")
            {
                json payload; 
                payload["command"] = "private_msg";
                payload["text"] = message_data["text"];
                payload["user_from"] = user_data->id;
                ws->publish("user" + to_string(message_data["user_to"]), payload.dump()); //Отправляем получателю
                std::cout << "User sent private message: " << user_data->id << " to: " 
                          << message_data["user_to"] << std::endl;
            }
            if (message_data["command"] == "SET_NAME")
            {
                json payload;
                payload["command"] = "SET_NAME";
                if (message_data["text"].size() < 256)
                {
                    user_data->name = message_data["text"];
                    payload["user_id"] = user_data->id;
                    payload["user_name"] = user_data->name;
                    ws->publish("public_chanel", payload.dump());
                    std::cout << "User " << user_data->id << " added name: " << user_data->name << std::endl;
                }
                else
                {
                    std::cout << "User " << user_data->id << " error name " << std::endl;
                }
            }
            //
            //Планируем дать пользователю следующие возможности
            //1. Отправлять публичные сообщения всем на сервер
                // Обработать ошибку из-за неверного формата
                //Пользователь серверу: {"command": "public_msg", "text": "Всем привет!"}
                //Сервер пользователям: {"command": "public_msg", "text": "Всем привет!", "user_id": 11}
            //2. Отправлять приватные сообщения кому-то конкретному
                //Пользователь серверу: {"command": "private_msg", "text": "Привет тебе 11ый!", "user_to": 11}
                //Сервер пользователям: {"command": "private_msg", "text": "Привет тебе 11ый!", "user_from": 11}
            //3. Указать свое имя 
        }

    }).listen(9001, 
        [](auto* listenSocket) 
        { 
            // http://127.0.0.1:9001
            if (listenSocket) {
                std::cout << "Listening on port " << 9001 << std::endl;
        }
    }).run();
}

