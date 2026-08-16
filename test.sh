#!/usr/bin/env bash

# Impostazioni colore per l'output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Nome del tuo eseguibile (modificalo se il tuo binario si chiama diversamente)
EXECUTABLE="./bin/ccjq"
TESTS_DIR="integration_tests"

# Verifichiamo che l'eseguibile esista
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}Errore: Eseguibile '$EXECUTABLE' non trovato! Compila prima il progetto.${NC}"
    exit 1
fi

PASSED=0
FAILED=0

echo "=========================================="
echo -e "${BOLD}       SUITE DI TEST JSON PARSER          ${NC}"
echo "=========================================="

# Scorriamo tutte le cartelle step1, step2, step3, step4
for step_dir in "$TESTS_DIR"/step*/; do
    step_name=$(basename "$step_dir")
    echo ""
    echo -e "${BOLD}--- Testing $step_name ---${NC}"

    for test_file in "$step_dir"/*.json; do
        # Se la wildcard non trova file, passa oltre
        [ -e "$test_file" ] || continue

        filename=$(basename "$test_file")

        # Eseguiamo il parser sul file nascondendo l'output di stdout e stderr
        "$EXECUTABLE" "." "$test_file" > /dev/null 2>&1
        exit_code=$?

        # Valutazione del risultato in base al nome del file
        if [[ "$filename" == valid* ]]; then
            # File valido: ci aspettiamo exit code 0
            if [ $exit_code -eq 0 ]; then
                echo -e "[ ${GREEN}PASS${NC} ] $step_name/$filename (Atteso: Valido, Esito: OK)"
                ((PASSED++))
            else
                echo -e "[ ${RED}FAIL${NC} ] $step_name/$filename (Atteso: Valido, ma ha restituito codice $exit_code)"
                ((FAILED++))
            fi
        elif [[ "$filename" == invalid* ]]; then
            # File non valido: ci aspettiamo exit code != 0
            if [ $exit_code -ne 0 ]; then
                echo -e "[ ${GREEN}PASS${NC} ] $step_name/$filename (Atteso: Invalido, Esito: Rifiutato)"
                ((PASSED++))
            else
                echo -e "[ ${RED}FAIL${NC} ] $step_name/$filename (Atteso: Invalido, ma è stato considerato Valido)"
                ((FAILED++))
            fi
        fi
    done
done

echo ""
echo "=========================================="
if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}${BOLD}Risultati: $PASSED superati, $FAILED falliti! Great job! 🎉${NC}"
else
    echo -e "${RED}${BOLD}Risultati: $PASSED superati, $FAILED falliti.${NC}"
fi
echo "=========================================="

# Se c'è almeno un test fallito, lo script di test restituisce 1 (utile per CI/CD)
if [ $FAILED -ne 0 ]; then
    exit 1
fi
