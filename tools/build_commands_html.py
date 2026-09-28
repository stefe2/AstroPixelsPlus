#!/usr/bin/env python3
"""
Régénère docs/commands.html à partir de docs/commands.md.
Usage : python tools/build_commands_html.py (depuis n'importe quel répertoire)
Dépendance : pip install markdown
"""

import os

import markdown

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
SOURCE = os.path.join(ROOT, "docs", "commands.md")
TARGET = os.path.join(ROOT, "docs", "commands.html")


def main():
    with open(SOURCE, encoding="utf-8") as f:
        body = markdown.markdown(f.read(), extensions=["tables", "fenced_code"])

    # Garder l'en-tête et le style de la page existante
    with open(TARGET, encoding="utf-8") as f:
        page = f.read()
    head = page[: page.index("<body>")]

    with open(TARGET, "w", encoding="utf-8", newline="\n") as f:
        f.write(head)
        f.write('<body>\n    <div class="container">\n')
        f.write(body)
        f.write("\n    </div>\n</body>\n</html>\n")

    print(f"Généré : {os.path.relpath(TARGET, ROOT)}")


if __name__ == "__main__":
    main()
