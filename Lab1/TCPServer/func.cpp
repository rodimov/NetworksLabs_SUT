#pragma once

#include <QTextStream>

#include "func.h"

QTextStream& consoleOut() {
	static QTextStream textStream(stdout);
	return textStream;
}

QTextStream& consoleIn() {
	static QTextStream textStream(stdin);
	return textStream;
}
