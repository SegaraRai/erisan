#define NOMINMAX

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <iostream>
#include <io.h>
#include <fcntl.h>

#include <sakuraglx/sakuraglx.h>
#include <sakuragl/sgl_erisa_lib.h>

using namespace std::literals;


namespace {
  constexpr std::size_t BufferSize = 4096;

  enum class Mode {
    Decode,
    Encode,
  };


  int ShowUsage(const wchar_t* program) {
    std::wcerr << L"erisan v0.1.0"sv << std::endl;
    std::wcerr << L"Copyright (c) 2019 SegaraRai"sv << std::endl;
    std::wcerr << std::endl;
    std::wcerr << L"usage:"sv << std::endl;
    std::wcerr << program << L" e infile outfile"sv << std::endl;
    std::wcerr << program << L" d infile outfile"sv << std::endl;
    std::wcerr << std::endl;
    std::wcerr << L"e: encode"sv << std::endl;
    std::wcerr << L"d: decode"sv << std::endl;
    std::wcerr << std::endl;
    std::wcerr << L"set infile to \"-\" to input from stdin"sv << std::endl;
    std::wcerr << L"set outfile to \"-\" to output to stdout"sv << std::endl;
    std::wcerr << std::endl;
    std::wcerr << L"This program makes use of EntisGLS version 4s.05." << std::endl;
    std::wcerr << std::endl;
    std::wcerr << L"EntisGLS version 4s.05"sv << std::endl;
    std::wcerr << L"Copyright (c) 1998-2014 Leshade Entis, Entis soft."sv << std::endl;
    return 2;
  }
}


int xwmain(int argc, wchar_t* argv[]) {
  if (argc != 4) {
    return ShowUsage(argv[0]);
  }

  const std::wstring_view strMode(argv[1]);

  Mode mode;
  if (strMode == L"d"sv) {
    mode = Mode::Decode;
  } else if (strMode == L"e"sv) {
    mode = Mode::Encode;
  } else {
    return ShowUsage(argv[0]);
  }

  const std::wstring inFile(argv[2]);
  const std::wstring outFile(argv[3]);

  const bool useStdin = inFile == L"-"sv;
  const bool useStdout = outFile == L"-"sv;

  if (useStdin) {
    if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
      throw std::runtime_error("_setmode failed for stdin"s);
    }
  }

  if (useStdout) {
    if (_setmode(_fileno(stdout), _O_BINARY) == -1) {
      throw std::runtime_error("_setmode failed for stdout"s);
    }
  }

  SSystem::SFileInterface* inputFile = nullptr;
  SSystem::SFileInterface* outputFile = nullptr;

  try {
    inputFile = SSystem::SFileOpener::DefaultNewOpenFile(useStdin ? L"<stdin>" : inFile.c_str(), SSystem::SFileOpener::shareRead);
    outputFile = SSystem::SFileOpener::DefaultNewOpenFile(useStdout ? L"<stdout>" : outFile.c_str(), SSystem::SFileOpener::modeCreateFile);

    auto buffer = std::make_unique<std::byte[]>(BufferSize);

    switch (mode) {
      case Mode::Decode:
      {
        ERISA::SGLDecodeBitStream	bitStream(BufferSize);
        bitStream.AttachInputStream(inputFile);

        ERISA::SGLERISANDecodeContext	decodeContext(&bitStream);
        decodeContext.PrepareToDecodeERISANCode();

        int zeroCount = 0;
        while (!decodeContext.GetEOFFlag()) {
          const auto readSize = decodeContext.Read(buffer.get(), BufferSize);
          if (readSize == 0) {
            zeroCount++;
            if (zeroCount > 1) {
              // not sure if this is correct
              throw std::runtime_error("invalid input"s);
            }
            continue;
          }
          zeroCount = 0;
          outputFile->Write(buffer.get(), readSize);
        }

        break;
      }

      case Mode::Encode:
      {
        ERISA::SGLEncodeBitStream	bitStream(BufferSize);
        bitStream.AttachOutputStream(outputFile);

        ERISA::SGLERISANEncodeContext	encodeContext(&bitStream);
        encodeContext.PrepareToEncodeERISANCode();

        while (true) {
          const auto readSize = inputFile->Read(buffer.get(), BufferSize);
          if (readSize == 0) {
            // EOF
            break;
          }
          encodeContext.Write(buffer.get(), readSize);
        }

        encodeContext.EncodeERISANCodeEOF();
        encodeContext.FinishERISACode();

        bitStream.Flushout();

        break;
      }
    }
  } catch (...) {
    delete outputFile;
    delete inputFile;

    throw;
  }

  delete outputFile;
  delete inputFile;

  return 0;
}
