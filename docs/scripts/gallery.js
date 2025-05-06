let currentSlide = 0;
const slides = document.querySelectorAll('.slide');
const thumbnails = document.querySelectorAll('.thumbnail');
const prevBtn = document.querySelector('.prev');
const nextBtn = document.querySelector('.next');

function showSlide(index) {
  slides.forEach((slide, i) => {
    slide.classList.toggle('active', i === index);
  });

  thumbnails.forEach((thumb, i) => {
    thumb.classList.toggle('active', i === index);
  });

  currentSlide = index;
}

prevBtn.addEventListener('click', () => {
  const newIndex = (currentSlide - 1 + slides.length) % slides.length;
  showSlide(newIndex);
});

nextBtn.addEventListener('click', () => {
  const newIndex = (currentSlide + 1) % slides.length;
  showSlide(newIndex);
});

thumbnails.forEach(thumb => {
  thumb.addEventListener('click', () => {
    const index = parseInt(thumb.dataset.index);
    showSlide(index);
  });
});

// Initial display
showSlide(currentSlide);

