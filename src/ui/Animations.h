#pragma once

class QWidget;

namespace Animations {

// Dissolvenza in entrata, opzionalmente dopo un ritardo (per effetti "a cascata").
void fadeIn(QWidget *widget, int durationMs = 280, int delayMs = 0);

// Scossone orizzontale, per segnalare un errore.
void shake(QWidget *widget);

} // namespace Animations
