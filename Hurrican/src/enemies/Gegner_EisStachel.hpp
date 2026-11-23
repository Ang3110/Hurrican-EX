#ifndef _GEGNER_EISSTACHEL_HPP_
#define _GEGNER_EISSTACHEL_HPP_

#include "GegnerClass.hpp"
#include "Gegner_Stuff.hpp"

class GegnerEisStachel : public GegnerClass {
    float patrolCenter = 0.0f;
    float patrolRadius = -1.0f;
    float patrolPhase = 0.0f;

  public:
    GegnerEisStachel(float x, float y, int Wert1, int Wert2, bool Light);
    void GegnerExplode() override;  // Gegner explodiert
    void DoKI() override;           // Gegner individuell mit seiner eigenen kleinen KI bewegen
};

#endif
