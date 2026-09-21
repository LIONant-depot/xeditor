#ifndef XEDITOR_SERIALIZE_H
#define XEDITOR_SERIALIZE_H
#pragma once

#include "dependencies/xundo/source/xundo_system.h"

#include <cstdint>
#include <string>

namespace xeditor
{
    // A length-prefixed string inside an xundo undo_file record.
    inline void WriteString(xundo::undo_file& File, const std::string& S) noexcept
    {
        const std::uint32_t Len = static_cast<std::uint32_t>(S.size());
        File.Write(Len);
        if (Len) File.Write(S.data(), Len);
    }

    inline std::string ReadString(xundo::undo_file& File) noexcept
    {
        std::uint32_t Len = 0; File.Read(Len);
        std::string S; S.resize(Len);
        if (Len) File.Read(S.data(), Len);
        return S;
    }

    // Base64 for arbitrary text (property paths and values) inside a space-delimited command line: plain ids need no encoding.
    inline std::string Base64Encode(const std::string& In) noexcept
    {
        static constexpr char Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string Out;
        Out.reserve(((In.size() + 2) / 3) * 4);
        std::size_t i = 0;
        for (; i + 2 < In.size(); i += 3)
        {
            const std::uint32_t N = (std::uint32_t(std::uint8_t(In[i])) << 16) | (std::uint32_t(std::uint8_t(In[i + 1])) << 8) | std::uint8_t(In[i + 2]);
            Out += Alphabet[(N >> 18) & 0x3F]; Out += Alphabet[(N >> 12) & 0x3F];
            Out += Alphabet[(N >> 6) & 0x3F];  Out += Alphabet[N & 0x3F];
        }
        const std::size_t Rem = In.size() - i;
        if (Rem == 1)
        {
            const std::uint32_t N = std::uint32_t(std::uint8_t(In[i])) << 16;
            Out += Alphabet[(N >> 18) & 0x3F]; Out += Alphabet[(N >> 12) & 0x3F]; Out += "==";
        }
        else if (Rem == 2)
        {
            const std::uint32_t N = (std::uint32_t(std::uint8_t(In[i])) << 16) | (std::uint32_t(std::uint8_t(In[i + 1])) << 8);
            Out += Alphabet[(N >> 18) & 0x3F]; Out += Alphabet[(N >> 12) & 0x3F]; Out += Alphabet[(N >> 6) & 0x3F]; Out += '=';
        }
        return Out;
    }

    inline std::string Base64Decode(const std::string& In) noexcept
    {
        auto DecodeChar = [](char C) -> int
        {
            if (C >= 'A' && C <= 'Z') return C - 'A';
            if (C >= 'a' && C <= 'z') return C - 'a' + 26;
            if (C >= '0' && C <= '9') return C - '0' + 52;
            if (C == '+') return 62;
            if (C == '/') return 63;
            return -1; // padding ('=') or terminator
        };
        std::string Out;
        Out.reserve((In.size() / 4) * 3);
        int Bits = 0, NumBits = 0;
        for (char C : In)
        {
            const int V = DecodeChar(C);
            if (V < 0) break;
            Bits = (Bits << 6) | V;
            NumBits += 6;
            if (NumBits >= 8)
            {
                NumBits -= 8;
                Out += static_cast<char>((Bits >> NumBits) & 0xFF);
            }
        }
        return Out;
    }
}

#endif
