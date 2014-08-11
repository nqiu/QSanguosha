/********************************************************************
    Copyright (c) 2013-2014 - QSanguosha-Hegemony Team

    This file is part of QSanguosha-Hegemony.

    This game is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3.0 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    See the LICENSE file for more details.

    QSanguosha-Hegemony Team
    *********************************************************************/

#include "protocol.h"

#include <json/json.h>
#include <QString>

using namespace std;
using namespace QSanProtocol;

unsigned int QSanProtocol::Packet::globalSerialSequence = 0;
const unsigned int QSanProtocol::Packet::S_MAX_PACKET_SIZE = 65535;
const char *QSanProtocol::S_PLAYER_SELF_REFERENCE_ID = "MG_SELF";

const int QSanProtocol::S_ALL_ALIVE_PLAYERS = 0;

bool QSanProtocol::Countdown::tryParse(const Json::Value &val) {
    if (!val.isArray())
        return false;

    //compatible with old JSON representation of Countdown
    Json::ArrayIndex offset = 0;
    if (val[0].isString()) {
        if (val[0].asString() == "MG_COUNTDOWN")
            offset = 1;
        else
            return false;
    }

    if (val.size() - offset == 2) {
        if (!Utils::isIntArray(val, offset, offset + 1)) return false;
        current = (time_t)val[offset].asInt();
        max = (time_t)val[offset + 1].asInt();
        type = S_COUNTDOWN_USE_SPECIFIED;
        return true;
    }
    else if (val.size() - offset == 1 && val[offset].isInt()) {
        CountdownType type = (CountdownType)val[offset].asInt();
        if (type != S_COUNTDOWN_NO_LIMIT && type != S_COUNTDOWN_USE_DEFAULT)
            return false;
        else this->type = type;
        return true;
    }
    else
        return false;
}

bool QSanProtocol::Utils::isStringArray(const Json::Value &jsonObject, unsigned int startIndex, unsigned int endIndex) {
    if (!jsonObject.isArray() || jsonObject.size() <= endIndex)
        return false;
    for (unsigned int i = startIndex; i <= endIndex; i++) {
        if (!jsonObject[i].isString())
            return false;
    }
    return true;
}

bool QSanProtocol::Utils::isIntArray(const Json::Value &jsonObject, unsigned int startIndex, unsigned int endIndex) {
    if (!jsonObject.isArray() || jsonObject.size() <= endIndex)
        return false;
    for (unsigned int i = startIndex; i <= endIndex; i++) {
        if (!jsonObject[i].isInt())
            return false;
    }
    return true;
}

QSanProtocol::Packet::Packet(int packetDescription, CommandType command)
    : globalSerial(0), localSerial(0),
    command(command),
    packetDescription(static_cast<PacketDescription>(packetDescription)),
    messageBody(Json::nullValue)
{
}

unsigned int QSanProtocol::Packet::createGlobalSerial() {
    globalSerial = ++globalSerialSequence;
    return globalSerial;
}

bool QSanProtocol::Packet::parse(const string &s) {
    if (s.length() > S_MAX_PACKET_SIZE) {
        return false;
    }

    Json::Value result;

    Json::Reader reader;
    bool success = reader.parse(s, result);
    if (!success || !Utils::isIntArray(result, 0, 3) || result.size() > 5)
        return false;

    globalSerial = result[0].asUInt();
    localSerial = result[1].asUInt();
    packetDescription = static_cast<PacketDescription>(result[2].asInt());
    command = (CommandType)result[3].asInt();

    if (result.size() == 5)
        messageBody = result[4];
    return true;
}


//characters in JSON string representations are unicode-escaped. So we don't need Base64 here.
QByteArray QSanProtocol::Packet::toUtf8() const{
    Json::Value result(Json::arrayValue);
    result[0] = globalSerial;
    result[1] = localSerial;
    result[2] = packetDescription;
    result[3] = command;
    if (messageBody != Json::nullValue)
        result[4] = messageBody;

    Json::FastWriter writer;
    string msg = writer.write(result);

    //truncate too long messages
    if (msg.length() > S_MAX_PACKET_SIZE)
        msg = msg.substr(0, S_MAX_PACKET_SIZE);

    return msg.c_str();
}

QString QSanProtocol::Packet::toString() const{
    return QString::fromUtf8(toUtf8());
}

