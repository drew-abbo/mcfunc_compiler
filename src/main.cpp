#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <ostream>
#include <vector>

#include <compiler/compile_error.h>
#include <compiler/sourceFiles.h>
#include <compiler/syntax_analysis/statement.h>
#include <compiler/tokenization/Token.h>

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

int main() {

  auto startTime = std::chrono::high_resolution_clock::now();

  sourceFiles.emplace_back("test.mcfunc");
  sourceFiles.emplace_back("foo.mcfunc");

  // try and do syntax analysis the file
  try {
    sourceFiles.evaluateAll();
    sourceFiles.link();
  } catch (const compile_error::Generic& e) {
    std::cout << "COMPILATION FAILED" << std::endl;
    std::cout << e.what();
    return EXIT_FAILURE;
  }

  auto endTime = std::chrono::high_resolution_clock::now();

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

  // print the time it took
  std::chrono::duration<double> timeTaken = endTime - startTime;
  auto timeTakenMil = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
  std::cout << "Compile time: " << timeTaken.count() << " seconds (" << timeTakenMil.count()
            << " milliseconds)." << std::endl;
}
