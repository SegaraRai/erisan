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

  void WriteFileFromStream(SSystem::SFileInterface* destFile, SSystem::SInputStream* srcStream) {
    
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

  auto inputFile = SSystem::SFileOpener::DefaultNewOpenFile(useStdin ? L"<stdin>" : inFile.c_str(), SSystem::SFileOpener::shareRead);
  auto outputFile = SSystem::SFileOpener::DefaultNewOpenFile(useStdout ? L"<stdout>" : outFile.c_str(), SSystem::SFileOpener::modeCreateFile);

  switch (mode) {
    case Mode::Decode:
    {
      ERISA::SGLDecodeBitStream	bitStream(BufferSize);
      bitStream.AttachInputStream(inputFile);

      ERISA::SGLERISANDecodeContext	decoder(&bitStream);
      decoder.PrepareToDecodeERISANCode();

      auto buffer = std::make_unique<std::byte[]>(BufferSize);

      std::size_t writtenSize = 0;
      while (!decoder.GetEOFFlag()) {
        const auto readSize = decoder.Read(buffer.get(), BufferSize);
        outputFile->Write(buffer.get(), readSize);
        writtenSize += readSize;
      }

      break;
    }

    case Mode::Encode:
      ERISA::SGLEncodeBitStream	bitStream(BufferSize);
      bitStream.AttachOutputStream(outputFile);

      ERISA::SGLERISANEncodeContext	encoder(&bitStream);
      encoder.PrepareToEncodeERISANCode();

      auto buffer = std::make_unique<std::byte[]>(BufferSize);

      const auto srcSize = inputFile->GetLength();
      if (srcSize < 0) {
        throw std::runtime_error("invalid srcSize"s);
      }

      std::int64_t totalReadSize = 0;
      std::size_t writtenSize = 0;
      while (totalReadSize != srcSize) {
        const auto readSize = std::min(srcSize - totalReadSize, static_cast<std::int64_t>(BufferSize));
        inputFile->Read(buffer.get(), readSize);
        totalReadSize += readSize;
        writtenSize += encoder.Write(buffer.get(), static_cast<std::size_t>(readSize));
      }
      encoder.EncodeERISANCodeEOF();
      encoder.FinishERISACode();

      bitStream.Flushout();

      break;
  }

  delete outputFile;
  delete inputFile;

  return 0;
}
