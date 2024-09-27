#include "cli/style_text.h"
#include <cstdlib>
#include <iostream>

#include <cli/outputIsTerminal.h>
#include <cli/parseArgs.h>
#include <compiler/compile_error.h>

// set to 1 to enable compilation result printing
#define DO_DEBUG_PRINT_COMPILATION_RESULT 0
#if DO_DEBUG_PRINT_COMPILATION_RESULT

#include <cassert>
#include <memory>
#include <ostream>
#include <vector>

// print tokens in a somewhat readable way
void printTokens(const std::vector<Token> tokens) {
  std::string indent;
  for (const auto& t : tokens) {
    std::cout << tokenDebugStr(t);
    if (t.kind() == Token::SEMICOLON)
      std::cout << '\n' << indent;
    else if (t.kind() == Token::L_BRACE) {
      indent += '\t';
      std::cout << '\n' << indent;
    } else if (t.kind() == Token::R_BRACE) {
      indent.pop_back();
      std::cout << '\n' << indent;
    } else
      std::cout << ' ';
  }
}

void printStatement(const std::unique_ptr<statement::Generic>& statement,
                    const SourceFile& sourceFile, std::string indent) {
  switch (statement->kind()) {

  case statement::Kind::FUNCTION_CALL: {
    const auto& funcPtr = dynamic_cast<statement::FunctionCall*>(statement.get());

    const std::string funcName = sourceFile.tokens()[funcPtr->functionNameTokenIndex()].contents();
    std::cout << indent << funcName << "();\n";
    break;
  }

  case statement::Kind::COMMAND: {
    const auto& cmdPtr = dynamic_cast<statement::Command*>(statement.get());

    const std::string commandContents =
        sourceFile.tokens()[cmdPtr->commmandContentsTokenIndex()].contents();

    std::cout << indent << '/' << commandContents;

    if (cmdPtr->hasStatementAfterRun()) {
      std::cout << ":\n";
      printStatement(cmdPtr->statementAfterRun(), sourceFile, indent + "  ");
    } else
      std::cout << ";\n";
    break;
  }

  case statement::Kind::SCOPE: {
    const auto& scopePtr = dynamic_cast<statement::Scope*>(statement.get());
    std::cout << indent << "{";
    if (scopePtr->numTokens() == 2) {
      std::cout << "}\n";
      break;
    }
    std::cout << "\n";
    for (const auto& subStatement : scopePtr->statements())
      printStatement(subStatement, sourceFile, indent + "  ");
    std::cout << indent << "}\n";
    break;
  }
  }
}

void reconstructSyntaxAndPrint(const SourceFile& sourceFile) {
  // namespace
  if (sourceFile.namespaceExposeSymbol().isSet())
    std::cout << "expose \"" << sourceFile.namespaceExposeSymbol().exposedNamespace() << "\";\n\n";

  // imports
  for (const auto& symbol : sourceFile.importSymbolTable()) {
    std::cout << "import \"" << symbol.importPathToken().contents()
              << "\";\t// file: " << symbol.actualPath() << ", import as " << symbol.importPath()
              << "\n";
  }
  std::cout << '\n';

  // file writes
  for (const auto& symbol : sourceFile.fileWriteSymbolTable()) {
    std::cout << "// file: " << symbol.relativeOutPath() << "\n"
              << "file \"" << symbol.relativeOutPathToken().contents() << "\"";
    if (symbol.hasContents()) {
      const char contentStrChar = ((symbol.contentsToken().kind() == Token::STRING) ? '"' : '`');
      std::cout << " = " << contentStrChar << symbol.contents() << contentStrChar;
    }
    std::cout << ";\n\n";
  }

  for (const auto& symbol : sourceFile.functionSymbolTable()) {
    if (symbol.isPublic())
      std::cout << "public ";
    if (symbol.isTickFunc())
      std::cout << "tick ";
    if (symbol.isLoadFunc())
      std::cout << "load ";
    std::cout << "void " << symbol.name() << "()";
    if (symbol.isExposed())
      std::cout << " expose \"" << symbol.exposeAddress() << "\"";

    if (!symbol.isDefined()) {
      std::cout << ";\n\n";
      continue;
    }

    std::cout << " {\n";
    for (const auto& subStatement : symbol.definition().statements()) {
      printStatement(subStatement, sourceFile, "  ");
    }
    std::cout << "}\n\n";
  }
}

void printFullCompilationResult(const std::filesystem::path& outputDirectory,
                                const SourceFiles& sourceFiles,
                                const std::vector<FileWriteSourceFile>& fileWriteSourceFiles) {

  for (const auto& s : sourceFiles) {
    std::cout << "--->   " << s.importPath() << "   <---\n\n";
    reconstructSyntaxAndPrint(s);

    if (!s.unresolvedFunctionNames().empty()) {
      std::cout << "Unresolved Functions:\n";
      for (const auto& fName : s.unresolvedFunctionNames()) {
        std::cout << '\t' << fName << "()\n";
      }
      std::cout << '\n';
    }
  }

  std::cout << "File-Write Source Files:\n";
  for (const auto& fileWriteSourceFile : fileWriteSourceFiles) {
    std::cout << "- " << fileWriteSourceFile.path() << "\timportable as "
              << fileWriteSourceFile.importPath() << '\n';
  }
  std::cout << "\n";

  std::cout << "(!) Output Directory: " << outputDirectory << '\n';
}

#define DEBUG_PRINT_COMPILATION_RESULT(_outputDirectory, _sourceFiles, _fileWriteSourceFiles)      \
  printFullCompilationResult(_outputDirectory, _sourceFiles, _fileWriteSourceFiles);

#else

#define DEBUG_PRINT_COMPILATION_RESULT(_a, _b, _c)

#endif

int main(int argc, const char** argv) {
  // disable color/styled printing if we're outputting to a file or piping to
  // another process
  if (!outputIsTerminal())
    style_text::doColor = false;

  try {
    auto [outputDirectory, sourceFiles, fileWriteSourceFiles] = parseArgs(argc, argv);

    sourceFiles.evaluateAll();
    LinkResult linkResult = sourceFiles.link(fileWriteSourceFiles);
    linkResult.generateDataPack(outputDirectory);

    DEBUG_PRINT_COMPILATION_RESULT(outputDirectory, sourceFiles, fileWriteSourceFiles);

  } catch (const compile_error::Generic& e) {
    std::cerr << e.what();
    return EXIT_FAILURE;
  }
}
