// 要点：
// 1. Ace可以算1或11
// 2. J, Q, K算10
// 3. 其他按牌面数字
// 4. 最优点数（不超过21的情况下尽量大）

int calculate_points(int cards[], int count) {
    int points = 0;
    int ace_count = 0;
    
    for (int i = 0; i < count; i++) {
        int card = cards[i];
        
        if (card == 1) {  // Ace
            ace_count++;
            points += 11;  // 先算11
        } else if (card >= 10) {  // 10, J, Q, K
            points += 10;
        } else {
            points += card;
        }
    }
    
    // 如果爆牌且有Ace，把Ace从11变成1
    while (points > 21 && ace_count > 0) {
        points -= 10;  // 11变1，所以减10
        ace_count--;
    }
    
    return points;
}
