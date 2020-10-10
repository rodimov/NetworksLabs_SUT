#include "func.h"

#include <QTextStream>

QTextStream& consoleOut() {
	static QTextStream textStream(stdout);
	return textStream;
}

QTextStream& consoleIn() {
	static QTextStream textStream(stdin);
	return textStream;
}
