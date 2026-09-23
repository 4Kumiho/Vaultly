#pragma once

class QApplication;

// Tema scuro dell'app: palette, font e foglio di stile (QSS).
// I widget scelgono il loro aspetto con proprietà dinamiche lette dal QSS:
//   QLabel      role    = brand | title | subtitle | sectionTitle | fieldLabel | caption | muted | hint | balance
//                         | cardTitle | cardAmount | rowTitle | rowAmount | statAmount | error
//   QLabel      tone    = positive | negative (importi in verde / rosso)
//   QPushButton variant = primary | secondary | link | danger | chip | segment (tone = income | expense)
//   QLineEdit   emphasis = large   (non "size": è già una proprietà di QWidget)
//   QFrame      objectName = card | accountCard | newAccountCard | drawer | segmented | listRow | divider,
//               selected = true | false
//   QLabel      objectName = rowIcon (tone = positive | negative | accent) | toast
namespace Theme {

void apply(QApplication &app);

} // namespace Theme
