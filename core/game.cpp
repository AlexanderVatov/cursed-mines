#include "game.hpp"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <queue>
#include <set>
#include <stdexcept>

Game::Game(std::size_t height, std::size_t width): Grid<Square>(height, width) {
    _state = NotStarted;
}

unsigned Game::numberOfMines() const {
    return _mines;
}

unsigned Game::numberOfFlags() const {
    return _flags;
}

void Game::create(unsigned numberOfMines, std::size_t revealedY, std::size_t revealedX) {
    if(_mines != 0) clear();
    
    std::set<std::size_t> locationsToAvoid;
    locationsToAvoid.insert(flatIndex(revealedY, revealedX));
    
    auto neighborhood = neighbors(revealedY, revealedX);
    for(auto it = neighborhood.begin(); it != neighborhood.end(); ++it)
        locationsToAvoid.insert(it.flatIndex());
    
    plantMines(numberOfMines, locationsToAvoid);
    reveal(revealedY, revealedX);
}


Game::State Game::reveal(std::size_t y, std::size_t x) {
    if(_state == NotStarted) start();
    
    //The non-recursive version of the flood fill algorithm (sometimes called forest fire) is used
    std::queue<std::size_t> q;
    Neighborhood neighborhood = neighbors(y, x);
    std::size_t clickedIndex = flatIndex(y,x);
    Square & clickedSquare = at(clickedIndex);
    if(clickedSquare.state() == Square::Open) {
        return _state; // Nothing changed
    }
    ++_openSquares;
    clickedSquare.setState(Square::Open);
    if(clickedSquare.isMined()) {
        _state = Lost;
        return _state;
    }
    
    detectWin();
    
    if(clickedSquare.surroundingMines() != 0) return _state;
    
    q.push(clickedIndex); //Push the clicked square to q
    for(Neighborhood::Iterator it = neighborhood.begin(); it != neighborhood.end(); ++it)
        q.push(it.flatIndex());
    
    for(; !q.empty(); q.pop()) {
        size_t currentIndex = q.front();
        Square const& currentSquare = get(currentIndex);
        if(currentSquare.surroundingMines() != 0) continue;
        
        auto neighborsNeighborhood = neighbors(currentIndex);
        for(auto it = neighborsNeighborhood.begin(); it != neighborsNeighborhood.end(); ++it) {
            if(it->state() == Square::Closed) {
                ++_openSquares;
                it->setState(Square::Open);
                q.push(it.flatIndex());
            }
        }
    }
    detectWin();
    return _state;
}

Game::State Game::revealUnflaggedNeighbors ( std::size_t y, std::size_t x ) {
    
    auto neighborhood = neighbors(y, x);
    for(auto it = neighborhood.begin(); it != neighborhood.end(); ++it) {
        if(it->state() == Square::Closed) {
            Position pos = it.pos();
            reveal(pos.row, pos.col);
        }
    }
    detectWin();
    return _state;
}


Game::State Game::toggleFlag(std::size_t y, std::size_t x) {
    Square & sq = at(y, x);
    switch(sq.state()) {
        case Square::Closed:
        case Square::QuestionMark:
            sq.setState(Square::Flagged);
            ++_flags;
            break;
        case Square::Flagged:
            sq.setState(Square::Closed);
            --_flags;
            break;
        case Square::Open:
            break;
    }
    detectWin();
    return _state;
}

void Game::start() {
    _state = Running;
}

void Game::clear() {
    std::fill(Grid<Square>::begin(), Grid<Square>::end(), Square());
    _state = NotStarted;
    _openSquares = 0;
    _mines = 0;
    _flags = 0;
}

Square const * Game::begin() const {
    return Grid<Square>::begin();
}

Square const * Game::end() const {
    return Grid<Square>::end();
}

Square const * Game::row_begin(std::size_t row) const {
    return Grid<Square>::row_begin(row);
}

Square const * Game::row_end(std::size_t row) const {
    return Grid<Square>::row_end(row);
}

void Game::plantMines(std::set<std::size_t> const & locations) {
    for(size_t ind: locations) {
        Square& sq = at(ind);
        if(!sq.isMined()) {
            sq.setMined(true);
            for(Square& n: neighbors(ind)) n.incrementSurroundingMines();
        }
    }
    
    _mines += locations.size();
};

void Game::plantMines(unsigned int number, std::set<std::size_t> const& locationsToAvoid) {
    std::set<std::size_t> locations;
    std::size_t currentRandom;
    std::size_t size = height() * width();
    
    std::srand(std::time(nullptr));
    for(int i = 1; i <= number; ++i) {
        do {
            currentRandom = std::rand() % size;
        } while(locations.count(currentRandom) != 0 || locationsToAvoid.count(currentRandom) != 0);
        locations.insert(currentRandom);
    }
    
    plantMines(locations);
}

void Game::detectWin() {
    if (_state == Running) 
        if((_openSquares == width()*height() - _mines) && (_flags == _mines))
            _state = Won;
}
