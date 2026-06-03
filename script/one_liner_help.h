#pragma once

#include <QtGlobal>

class QDialog;
class QWidget;

namespace OneLinerHelp {

QDialog* createHelpDialog(QWidget* parent, qreal scale);

} // namespace OneLinerHelp