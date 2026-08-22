// Support Modal functionality
document.addEventListener('DOMContentLoaded', function() {
    // Donation Modal
    const openDonationBtn = document.getElementById('openDonationModal');
    const donationModal = document.getElementById('donationModal');
    const closeModal = document.getElementById('closeModal');

    if (openDonationBtn && donationModal && closeModal) {
        openDonationBtn.addEventListener('click', function() {
            // Закрываем основное окно поддержки перед открытием дочернего
            const supportModal = document.getElementById('supportModal');
            if (supportModal && supportModal.classList.contains('active')) {
                supportModal.classList.remove('active');
            }
            donationModal.classList.add('active');
        });

        closeModal.addEventListener('click', function() {
            donationModal.classList.remove('active');
        });

        donationModal.addEventListener('click', function(e) {
            if (e.target === donationModal) {
                donationModal.classList.remove('active');
            }
        });
    }

    // Crypto Modal
    const openCryptoBtn = document.getElementById('openCryptoModal');
    const cryptoModal = document.getElementById('cryptoModal');
    const closeCryptoModal = document.getElementById('closeCryptoModal');

    if (openCryptoBtn && cryptoModal && closeCryptoModal) {
        openCryptoBtn.addEventListener('click', function() {
            // Закрываем основное окно поддержки перед открытием дочернего
            const supportModal = document.getElementById('supportModal');
            if (supportModal && supportModal.classList.contains('active')) {
                supportModal.classList.remove('active');
            }
            cryptoModal.classList.add('active');
        });

        closeCryptoModal.addEventListener('click', function() {
            cryptoModal.classList.remove('active');
        });

        cryptoModal.addEventListener('click', function(e) {
            if (e.target === cryptoModal) {
                cryptoModal.classList.remove('active');
            }
        });
    }

    // Card Modal
    const openCardBtn = document.getElementById('openCardModal');
    const cardModal = document.getElementById('cardModal');
    const closeCardModal = document.getElementById('closeCardModal');

    if (openCardBtn && cardModal && closeCardModal) {
        openCardBtn.addEventListener('click', function() {
            // Закрываем основное окно поддержки перед открытием дочернего
            const supportModal = document.getElementById('supportModal');
            if (supportModal && supportModal.classList.contains('active')) {
                supportModal.classList.remove('active');
            }
            cardModal.classList.add('active');
        });

        closeCardModal.addEventListener('click', function() {
            cardModal.classList.remove('active');
        });

        cardModal.addEventListener('click', function(e) {
            if (e.target === cardModal) {
                cardModal.classList.remove('active');
            }
        });
    }

    // Crypto tabs
    const cryptoTabs = document.querySelectorAll('.crypto-tab');
    const cryptoContents = document.querySelectorAll('.crypto-content');

    cryptoTabs.forEach(tab => {
        tab.addEventListener('click', function() {
            const cryptoType = this.getAttribute('data-crypto');

            cryptoTabs.forEach(t => t.classList.remove('active'));
            cryptoContents.forEach(c => c.classList.remove('active'));

            this.classList.add('active');
            const activeContent = document.querySelector(`[data-crypto-content="${cryptoType}"]`);
            if (activeContent) {
                activeContent.classList.add('active');
            }
        });
    });

    // Copy address to clipboard
    const cryptoAddresses = document.querySelectorAll('.crypto-address');
    cryptoAddresses.forEach(address => {
        address.addEventListener('click', function() {
            const addressText = this.getAttribute('data-address');
            
            navigator.clipboard.writeText(addressText).then(() => {
                this.classList.add('copied');
                setTimeout(() => {
                    this.classList.remove('copied');
                }, 2000);
            }).catch(err => {
                console.error('Failed to copy:', err);
            });
        });
    });

    // Close modals on Escape key
    document.addEventListener('keydown', function(e) {
        if (e.key === 'Escape') {
            const supportModal = document.getElementById('supportModal');
            if (donationModal && donationModal.classList.contains('active')) {
                donationModal.classList.remove('active');
                if (supportModal) supportModal.classList.add('active');
            } else if (cryptoModal && cryptoModal.classList.contains('active')) {
                cryptoModal.classList.remove('active');
                if (supportModal) supportModal.classList.add('active');
            } else if (cardModal && cardModal.classList.contains('active')) {
                cardModal.classList.remove('active');
                if (supportModal) supportModal.classList.add('active');
            }
        }
    });
});
