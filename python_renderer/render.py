from weasyprint import HTML

def render():
    HTML("curriculum/optimized.html").write_pdf("resume.pdf")

if __name__ == "__main__":
    render()
