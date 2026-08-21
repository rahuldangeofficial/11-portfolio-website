const fs = require('fs');
let code = fs.readFileSync('src/components/Header.tsx', 'utf8');

// Replace the <nav> block
const navRegex = /<nav className="sticky top-0 z-50 bg-white border-b border-\[var\(--border-default\)\] py-2\.5">[\s\S]*?<\/nav>/;

const newNav = `<nav className="sticky top-0 z-50 bg-white border-b border-[var(--border-default)] py-2.5">
        <div className="max-w-4xl mx-auto px-5 sm:px-8 lg:px-8 flex items-center gap-4">
          <div className="flex items-center gap-2 text-sm font-semibold text-[var(--fg-primary)] shrink-0">
            <span>Rahul Dange</span>
          </div>

          <div className="flex items-center gap-1 text-[11px] sm:text-xs overflow-x-auto dark-scrollbar pb-1 sm:pb-0 w-full sm:w-auto">
            {navigationItems.map((item) => (
              <button
                key={item.id}
                onClick={() => scrollToSection(item.id)}
                className={\`px-2.5 py-1 rounded-full whitespace-nowrap transition-colors \${
                  activeSection === item.id
                    ? 'bg-[var(--fg-primary)] text-[var(--bg-primary)] font-bold'
                    : 'text-[var(--fg-secondary)] hover:text-[var(--fg-primary)] hover:bg-[var(--bg-surface)]'
                }\`}
              >
                {item.label}
              </button>
            ))}
          </div>
        </div>
      </nav>`;

code = code.replace(navRegex, newNav);
fs.writeFileSync('src/components/Header.tsx', code);
